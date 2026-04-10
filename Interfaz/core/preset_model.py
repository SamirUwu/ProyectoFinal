import json

class PresetModel:
    def __init__(self, name="Preset1"):
        self.name = name
        self.effects = []
        self.master_gain = 1.0

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
        self.master_gain = float(data.get("master_gain", 1.0))
        incoming_effects = data.get("effects", [])
        self.effects = []

        for index, effect in enumerate(incoming_effects):
            # FIX: si Flutter no manda id (o manda null), generar uno
            effect_id = effect.get("id") or f"fx_{index + 1}"

            self.effects.append({
                "id":      effect_id,
                "type":    effect["type"],
                "enabled": effect.get("enabled", True),
                "params":  effect.get("params", {})
            })

    def to_json(self):
        payload = {
            "command": "apply_preset",
            "name": self.name,
            "master_gain": self.master_gain,
            "effects": self.effects
        }
        return json.dumps(payload, indent=2)