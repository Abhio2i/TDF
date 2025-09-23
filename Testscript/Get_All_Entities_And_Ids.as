void main(ScriptEngine@ e) {
    array<string>@ ids = e.getAllEntityIds();
    Print("All IDs:");
    for (uint i = 0; i < ids.length(); i++) {
        Print(ids[i]);
    }

    array<Entity@>@ ents = e.getAllEntities();
    Print("All Entities:");
    for (uint i = 0; i < ents.length(); i++) {
        Print(ents[i].name);
    }
}

