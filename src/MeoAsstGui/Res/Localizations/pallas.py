import re
import random

f = open('pallas.xaml', encoding='utf8')
txt = f.read()
f.close()


def wine_and_dance(m):
    rep = ''
    for i in range(len(m.group(1)) + 1):
        rep += random.choice(['💃', '🕺',
                              '🍷', '🍸', '🍺', '🍻',
                              '🍷', '🍸', '🍺', '🍻'])
    return f'>{rep}</system:String>'


pattern = re.compile('>(.*)</system:String>')
out = pattern.sub(wine_and_dance, txt)

print(out)
