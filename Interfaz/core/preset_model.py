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

    def load_from_json(self, data):

        self.name = data.get("name", self.name)

        incoming_effects = data.get("effects", [])

        self.effects = []

        for index, effect in enumerate(incoming_effects):

            effect_with_id = {
                "id": index,  # 
                "type": effect["type"],
                "enabled": effect.get("enabled", True),
                "params": effect.get("params", {})
            }

            self.effects.append(effect_with_id)

    def to_json(self):
        payload = {
            "command": "apply_preset",
            "name": self.name,
            "effects": self.effects
        }
        return json.dumps(payload, indent=2)