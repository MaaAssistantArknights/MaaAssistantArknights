_PRECEDENCE = {
    '#u': 4,  # 单目运算符 #
    '@': 3,
    '#': 3,  # 双目运算符 #
    '*': 2,
    '+': 1,
    '^': 1
}

_ASSOCIATIVITY = {
    '#u': 'R',
    '@': 'L',
    '#': 'L',
    '*': 'L',
    '+': 'L',
    '^': 'L'
}

_TOKENS = [*_PRECEDENCE.keys(), '(', ')']


def _is_operator(token):
    return token in _PRECEDENCE


def _tokenize(expression):
    tokens = []
    i = 0
    while i < len(expression):
        if expression[i] in _TOKENS:
            if expression[i] == '#' and (i == 0 or expression[i - 1] in _TOKENS and expression[i - 1] != ')'):
                tokens.append('#u')  # Treat as unary operator
            else:
                tokens.append(expression[i])
            i += 1
        elif expression[i].isalpha():
            j = i
            while j < len(expression) and expression[j].isalpha():
                j += 1
            tokens.append(expression[i:j])
            i = j
        elif expression[i].isdigit():
            j = i
            while j < len(expression) and expression[j].isdigit():
                j += 1
            tokens.append(int(expression[i:j]))
            i = j
        else:
            raise ValueError(f'Invalid token: {expression[i]}')
    return tokens


def _shunting_yard(tokens):
    output = []
    stack = []
    for token in tokens:
        if token == '(':
            stack.append(token)
        elif token == ')':
            while stack and stack[-1] != '(':
                output.append(stack.pop())
            stack.pop()
        elif _is_operator(token):
            while stack and _is_operator(stack[-1]) and (
                    _PRECEDENCE[token] < _PRECEDENCE[stack[-1]] or
                    _PRECEDENCE[token] == _PRECEDENCE[stack[-1]] and
                    _ASSOCIATIVITY[token] == 'L'):
                output.append(stack.pop())
            stack.append(token)
        else:
            output.append(token)
    while stack:
        output.append(stack.pop())
    return output
