
with open('experiments/native-metal/engine/src/libs/sea_ai/src/ai_group.h', 'r', encoding='utf-8') as f:
    text = f.read()

getter = '''    const std::vector<AIShip *> &GetShips() const { return aGroupShips; }\n'''

assert 'class AIGroup : public VAI_GROUP' in text, 'AIGroup not found'
pos = text.find('public:')
text = text[:pos+7] + '\n    ' + getter + text[pos+7:]

with open('experiments/native-metal/engine/src/libs/sea_ai/src/ai_group.h', 'w', encoding='utf-8') as f:
    f.write(text)
print('Added GetShips getter to ai_group.h!')

