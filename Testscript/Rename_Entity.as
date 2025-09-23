void main(ScriptEngine@ e)
{
    // ✅ Use variables for ID and new name
    string entityId = "186696b521da1d328d5a95ef";
    string newName = "UMP";

    e.renameEntity(entityId, newName);
}

