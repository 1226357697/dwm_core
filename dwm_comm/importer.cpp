#include "importer.h"
#include <stdio.h>

#define ptr_add(ptr_type, base, off) ((ptr_type)((const char*)(base) + (off)))
const uint64_t fnv_prime = 1099511628211ULL;
const uint64_t fnv_offset_basis = 14695981039346656037ULL;

static char to_lower(char ch)
{
  return (ch >= 'A' && ch <= 'Z' ? (ch | (1 << 5)) : ch);
}

static const PEB* get_peb()
{
#if defined(_WIN64)
  return reinterpret_cast<const PEB*>(__readgsqword(0x60));
#else
  return reinterpret_cast<const PEB*>(__readfsdword(0x30));
#endif
}

hash_t string_hash(const char* string)
{
  hash_t hash = fnv_offset_basis;

  while (*string) {
    // XOR the byte with the current hash
    hash ^= (hash_t)(to_lower(*string));
    // Multiply by the FNV prime
    hash *= fnv_prime;
    string++;
  }

  return hash;
}

hash_t wstring_hash(const wchar_t* string)
{
  hash_t hash = fnv_offset_basis;

  while (*string) {
    // XOR the byte with the current hash
    hash ^= (hash_t)(to_lower (*string));
    // Multiply by the FNV prime
    hash *= fnv_prime;
    string++;
  }

  return hash;
}

hash_t unicode_string_hash(const UNICODE_STRING* string)
{
  hash_t hash = fnv_offset_basis;

  for (USHORT i = 0; i < (string->Length /sizeof(wchar_t)); ++i )
  {
    // XOR the byte with the current hash
    hash ^= (hash_t)(to_lower(string->Buffer[i]));
    // Multiply by the FNV prime
    hash *= fnv_prime;
  }

  return hash;
}



void* ldr_find_module(hash_t hash)
{
  const PEB* peb = get_peb();
  LDR_DATA_TABLE_ENTRY* head_entry = (LDR_DATA_TABLE_ENTRY*)peb->Ldr->InLoadOrderModuleList.Flink;
  for (LDR_DATA_TABLE_ENTRY* ldr_entry = (LDR_DATA_TABLE_ENTRY*)head_entry->InLoadOrderLinks.Flink;
    ldr_entry != head_entry;
    ldr_entry = (LDR_DATA_TABLE_ENTRY*)ldr_entry->InLoadOrderLinks.Flink)
  {
    if(ldr_entry->DllBase == NULL)  
      continue;

    if (unicode_string_hash(&ldr_entry->BaseDllName) == hash)
    {
      return ldr_entry->DllBase;
    }
  }
  return NULL;
}

void* ldr_find_function(void* dllbase, hash_t hash)
{
  if(dllbase == NULL)
    return NULL;

  IMAGE_DOS_HEADER* dos_hdr = (IMAGE_DOS_HEADER*)dllbase;
  IMAGE_NT_HEADERS* nt_hdr = (IMAGE_NT_HEADERS*)((char*)dllbase + dos_hdr->e_lfanew);
  if(dos_hdr->e_magic != IMAGE_DOS_SIGNATURE || nt_hdr->Signature != IMAGE_NT_SIGNATURE)
    return NULL;

  IMAGE_DATA_DIRECTORY* data_dir = &nt_hdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
  if(data_dir->Size == 0 || data_dir->VirtualAddress == 0)
    return NULL;

  IMAGE_EXPORT_DIRECTORY* export_dir =  (IMAGE_EXPORT_DIRECTORY*)((char*)dllbase + data_dir->VirtualAddress);
  int32_t* func_table = (int32_t*)((char*)dllbase + export_dir->AddressOfFunctions);
  int32_t* name_table = (int32_t*)((char*)dllbase + export_dir->AddressOfNames);
  int16_t* name_ordinal_table = (int16_t*)((char*)dllbase + export_dir->AddressOfNameOrdinals);

  if(export_dir->NumberOfFunctions == 0 || export_dir->NumberOfNames == 0)
    return NULL;  

  for (DWORD i = 0; i < export_dir->NumberOfNames; ++i)
  {
    const char* func_name =  ptr_add(const char*, dllbase, name_table[i]);
    if (string_hash(func_name) == hash)
    {
      int32_t func_index = name_ordinal_table[i];
      return ptr_add(void*, dllbase, func_table[func_index]);
    }
  }

  return NULL;
}
