setlocal commentstring=#\ %s
setlocal expandtab
setlocal shiftwidth=4
setlocal softtabstop=4
setlocal tabstop=4

if executable('walk')
  command! -buffer WalkFmt silent write | silent execute '!walk fmt -w ' . shellescape(expand('%:p')) | edit
endif
