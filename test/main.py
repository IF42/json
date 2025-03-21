import json

with open('large-file.json', "r", encoding="utf-8") as file:
    data = json.load(file)
    print(data[0]['actor'])



