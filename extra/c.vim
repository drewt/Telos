" Put this file in ~/.vim/after/syntax/
" for Telos syntax highlighting

" integer types
syn match s8_type display "s8"
syn match u8_type display "u8"
syn match s16_type display "s16"
syn match u16_type display "u16"
syn match s32_type display "s32"
syn match u32_type display "u32"
syn match s64_type display "s64"
syn match u64_type display "u64"
syn match uchar_type display "uchar"
syn match ushort_type display "ushort"
syn match ulong_type display "ulong"

hi def link s8_type Type
hi def link u8_type Type
hi def link s16_type Type
hi def link u16_type Type
hi def link s32_type Type
hi def link u32_type Type
hi def link s64_type Type
hi def link u64_type Type
hi def link uchar_type Type
hi def link ushort_type Type
hi def link ulong_type Type

" Telos '*_t' types
syn match pid_type display "pid_t"
syn match port_type display "port_t"
syn match dev_type display "dev_t"
syn match list_type display "list_t"
syn match list_head_type display "list_head_t"
syn match list_chain_type display "list_chain_t"
syn match list_entry_type display "list_entry_t"

hi def link pid_type Type
hi def link port_type Type
hi def link dev_type Type
hi def link list_type Type
hi def link list_head_type Type
hi def link list_chain_type Type
hi def link list_entry_type Type
