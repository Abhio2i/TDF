void main(ScriptEngine@ engine) 
{
    string entityId = "18665c7d0d06e03b54d95825";

    Entity@ ent = engine.getEntityById(entityId); // <- Entity@ is safe
    if (ent !is null) 
    {
        Print("Entity found: " + ent.name);
    } 
    else 
    {
        Print("Entity not found for ID: " + entityId);
    }
}

