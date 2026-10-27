void main(ScriptEngine@ e)
{
    e.useCity("Bhopal");

    e.AddCircle("Circle", 10000.0);

    e.moveShape("Circle", 22.7375, 75.8481);

    e.generatePDFReport("shape_move_report.pdf");
}