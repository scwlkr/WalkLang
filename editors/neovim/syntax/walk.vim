if exists('b:current_syntax')
  finish
endif

syntax keyword walkBoolean true false null
syntax keyword walkOperator and or not in
syntax match walkCommand '\v<(imp|exp|var|const|out|if|else|while|for|repeat|break|continue|func|return|test|assert|struct):'
syntax match walkType '\v<(int|float|bool|string|null|array|func|void)\??>'
syntax match walkNumber '\v<-?\d+(\.\d+)?>'
syntax match walkOperatorSymbol '\v(==|!=|>=|<=|[+\-*/^><:=.,?])'
syntax region walkString start=+'+ skip=+\\\\\|\\'+ end=+'+
syntax match walkComment '#.*$'

highlight default link walkBoolean Boolean
highlight default link walkCommand Keyword
highlight default link walkComment Comment
highlight default link walkNumber Number
highlight default link walkOperator Operator
highlight default link walkOperatorSymbol Operator
highlight default link walkString String
highlight default link walkType Type

let b:current_syntax = 'walk'
