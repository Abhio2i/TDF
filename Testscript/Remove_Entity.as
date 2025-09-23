void main(ScriptEngine@ e)
{
    string folderId = "18669a40f3759cf3e0c4ddd4";    // Parent folder ID
    string entityId = "18669a40f3a36b3237c0d5e7";    // Entity to remove

    e.removeEntity(folderId, entityId);
    Print("Entity removed successfully if it existed.");
}

