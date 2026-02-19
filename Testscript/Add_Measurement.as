void main(ScriptEngine@ e)
{
    e.canvasSetMeasurementUnit("kilometers");   // Unit set

    e.canvasStartDistanceMeasurement();          // Measurement start

    e.canvasAddMeasurePoint(77.2090, 24.6639);   // Point 1
    e.canvasAddMeasurePoint(77.4126, 23.2599);   // Point 2

    double d = e.canvasGetTotalDistance();       // Distance calculate

    // Generate PDF report
    e.generatePDFReport("measurement_report.pdf");
}
