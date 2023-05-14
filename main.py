import json

with open('large-file.json') as f:
    x = f.read()

y = json.loads(x)



