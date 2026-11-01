
with open('experiments/native-metal/engine/src/libs/ship/src/ship.cpp', 'r', encoding='utf-8') as f:
    text = f.read()

old_keys = '''    CONTROL_STATE gas, gas1, brake, brake1, shift, shift1;
    if (core.Controls)
    {
        core.Controls->GetControlState("Ship_SailUp", gas);
        core.Controls->GetControlState("Ship_SailUp1", gas1);
        core.Controls->GetControlState("Ship_SailDown", brake);
        core.Controls->GetControlState("Ship_SailDown1", brake1);
        core.Controls->GetControlState("ILeftShift", shift);
        core.Controls->GetControlState("IRightShift", shift1);
    }
    const bool isGas = gas.state == CST_ACTIVE || gas.state == CST_ACTIVATED ||
        gas1.state == CST_ACTIVE || gas1.state == CST_ACTIVATED;
    const bool isBrake = brake.state == CST_ACTIVE || brake.state == CST_ACTIVATED ||
        brake1.state == CST_ACTIVE || brake1.state == CST_ACTIVATED;
    const bool isShift = shift.state == CST_ACTIVE || shift.state == CST_ACTIVATED ||
        shift1.state == CST_ACTIVE || shift1.state == CST_ACTIVATED;'''

new_keys = '''    CONTROL_STATE gas, gas1, brake, brake1, shift, shift1;
    if (core.Controls)
    {
        core.Controls->GetControlState("Ship_SailUp", gas);
        core.Controls->GetControlState("Ship_SailUp1", gas1);
        core.Controls->GetControlState("Ship_SailDown", brake);
        core.Controls->GetControlState("Ship_SailDown1", brake1);
        core.Controls->GetControlState("ILeftShift", shift);
        core.Controls->GetControlState("IRightShift", shift1);
    }
    const Uint8 *sdlKeys = SDL_GetKeyboardState(nullptr);
    const bool isGas = gas.state == CST_ACTIVE || gas.state == CST_ACTIVATED ||
        gas1.state == CST_ACTIVE || gas1.state == CST_ACTIVATED || (sdlKeys && sdlKeys[SDL_SCANCODE_W]);
    const bool isBrake = brake.state == CST_ACTIVE || brake.state == CST_ACTIVATED ||
        brake1.state == CST_ACTIVE || brake1.state == CST_ACTIVATED || (sdlKeys && sdlKeys[SDL_SCANCODE_S]);
    const bool isShift = shift.state == CST_ACTIVE || shift.state == CST_ACTIVATED ||
        shift1.state == CST_ACTIVE || shift1.state == CST_ACTIVATED || (sdlKeys && (sdlKeys[SDL_SCANCODE_LSHIFT] || sdlKeys[SDL_SCANCODE_RSHIFT]));'''

assert old_keys in text, 'old_keys not found in ship.cpp'
text = text.replace(old_keys, new_keys, 1)

with open('experiments/native-metal/engine/src/libs/ship/src/ship.cpp', 'w', encoding='utf-8') as f:
    f.write(text)
print('Direct SDL scancodes added to ship.cpp Phonk boost!')

