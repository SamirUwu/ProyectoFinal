import json

class PresetModel:
    def __init__(self, name="Preset1"):
        self.name = name
        self.effects = []

    def set_effects(self, effects_list):
        self.effects = effects_list

    def update_param(self, effect_id, param, value):
        for effect in self.effects:
            if effect["id"] == effect_id:
                effect["params"][param] = value
                break

    def update_order(self, new_order):
        self.effects = new_order

    def toggle_effect(self, effect_index, state):
        self.effects[effect_index]["enabled"] = state

    def to_json(self):
        payload = {
            "command": "apply_preset",
            "name": self.name,
            "effects": self.effects
        }
        return json.dumps(payload, indent=2)