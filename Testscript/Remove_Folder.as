void main(ScriptEngine@ e)
{
    string profileId = "18669991d785b535bab6af1f";   // parent ID
    string folderId = "18669991d7c038ac50b5dc08";     // folder to remove

    e.removeFolder(profileId, folderId);
    Print("Folder removed successfully if it existed.");
}

