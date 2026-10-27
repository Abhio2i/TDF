void main(ScriptEngine@ e)
{
    e.SetMeasurementUnit("kilometers");

    e.StartDistanceMeasurement();

    e.AddMeasurePoint(24.6639, 77.2090);
    e.AddMeasurePoint(23.2599, 77.4126);

    double d = e.GetTotalDistance();

    e.generatePDFReport("measurement_report.pdf");
}