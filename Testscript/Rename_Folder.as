// Rename an existing folder
void main(ScriptEngine@ e) {
    // Replace with your folder ID
    string folderId = "18669bcf1a8c34a8491abe84";
    
    // New name
    string newName = "RenamedFolder";

    // Call wrapper
    e.renameFolder(folderId, newName);

    Print("Requested rename of folder " + folderId + " to: " + newName);
}

