void main(ScriptEngine@ e)
{
    // City context (if you already use this)
    e.useCity("Bhopal");

    // Add a circle
    e.canvasAddCircle("Circle", 10.0);

    // Move the newly created circle
    e.moveShape("TempCircle_0", 75.8481, 22.7375);

    e.generatePDFReport("shape_move_report.pdf");
}
