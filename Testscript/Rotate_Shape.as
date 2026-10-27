void main(ScriptEngine@ e)
{
    e.useCity("Bhopal");

    e.AddRectangle(
        "Grid_Block",
        40000.0,
        30000.0
    );

    e.sleep(3000);

    e.rotateShape(
        "Grid_Block",
        45.0
    );

    e.generatePDFReport(
        "GIS_Report_rotate.pdf"
    );
}