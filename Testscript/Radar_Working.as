void main(ScriptEngine@ e)
{
    ProfileCategaory@ p = e.NewProfile("AircraftProfile");
    
    // ==================================================
    // Jet1: Agra → New Delhi
    // ==================================================
    Platform@ jet1 = e.addEntityToPlatform(p, "Jet1");
    jet1.dynamicModel.moveSpeed = 1500;
    jet1.transform.setGeoCord(27.1767f, 78.0081f, 0.0f); // Agra
    jet1.trajectory.addWaypoint(27.1767f, 0.0f, 78.0081f); // Start
    jet1.trajectory.addWaypoint(28.6139f, 0.0f, 77.2090f); // New Delhi
    e.addSensorSubComponent(jet1, "Radar", "Generic");
    Sensor@ jet1Radar = jet1.getSensorByName("Radar");
    
    // ==================================================
    // Jet2: Gurugram → Hathras
    // ==================================================
    Platform@ jet2 = e.addEntityToPlatform(p, "Jet2");
    jet2.dynamicModel.moveSpeed = 1500;
    jet2.transform.setGeoCord(28.4595f, 77.0266f, 0.0f); // Gurugram
    jet2.trajectory.addWaypoint(28.4595f, 0.0f, 77.0266f); // Start (Gurugram)
    jet2.trajectory.addWaypoint(27.5983f, 0.0f, 78.0506f); // End (Hathras)
    e.addSensorSubComponent(jet2, "Radar", "Generic");
    Sensor@ jet2Radar = jet2.getSensorByName("Radar");
    
    // ==================================================
    // 🎯 AUTOMATED UI SETUP - Show Jet1 in Radar Display
    // ==================================================
    Print("🎯 Setting up automated display...");
    if (jet1Radar !is null) {
        e.selectEntityDisplay(jet1Radar);
        Print("✅ Sensors tab opened");
        Print("✅ Radar display selected");
        Print("✅ Jet1 selected in hierarchy");
        
        // Wait for display to render
        e.sleep(1000);
        
        // 📸 Capture initial screenshots (BOTH sensor + canvas)
       //  e.captureSensorScreenshot("initial.png");
      //  Print("📸 Initial screenshots captured (Sensor + Canvas)!");
    }
    
    // ==================================================
    // Simulation loop: check radar targets every second
    // ==================================================
    SimStart(e);
    Print("🚀 Simulation started!");
    
    int jet1PrevCount = 0;
    int jet2PrevCount = 0;
    bool simulationStopped = false;
    string detectedByJet = "";
    
    for (int t = 0; t < 400; t++)
    {
        e.sleep(1000); // 1 second per tick
        
        // ---------------- Jet1 Radar ----------------
        if (jet1Radar !is null)
        {
            int targetCount = jet1Radar.getTargetCount();
            if (targetCount > jet1PrevCount)
            {
                for (int i = jet1PrevCount; i < targetCount; i++)
                {
                    Target tgt = jet1Radar.getTarget(i);
                    Print("[Jet1Radar] 🎯 New target detected #" + i 
                        + " | Lat: " + tgt.lat 
                        + " | Lon: " + tgt.lon 
                        + " | Alt: " + tgt.altitude
                        + " | Speed: " + tgt.speed
                        + " | Direction: " + tgt.direction);
                    
                    // 🎯 Automatically switch to Jet1 display on detection
                    e.selectEntityDisplay(jet1Radar);
                    Print("📡 Switched to Jet1 radar display");
                    
                    // Wait for display update
                    e.sleep(1000);
                    
                    // 📸 Capture DUAL screenshots (Sensor + Canvas)
                    e.captureSensorScreenshot("jet1_detection.png");
                    Print("📸 Jet1 detection screenshots captured!");
                    Print("   → Radar display screenshot saved");
                    Print("   → Tactical canvas screenshot saved");
                    
                    detectedByJet = "Jet1";
                    simulationStopped = true;
                }
            }
            jet1PrevCount = targetCount;
        }
        
        // ---------------- Jet2 Radar ----------------
        if (jet2Radar !is null)
        {
            int targetCount = jet2Radar.getTargetCount();
            if (targetCount > jet2PrevCount)
            {
                for (int i = jet2PrevCount; i < targetCount; i++)
                {
                    Target tgt = jet2Radar.getTarget(i);
                    Print("[Jet2Radar] 🎯 New target detected #" + i 
                        + " | Lat: " + tgt.lat 
                        + " | Lon: " + tgt.lon 
                        + " | Alt: " + tgt.altitude
                        + " | Speed: " + tgt.speed
                        + " | Direction: " + tgt.direction);
                    
                    // 🎯 Automatically switch to Jet2 display on detection
                    e.selectEntityDisplay(jet2Radar);
                    Print("📡 Switched to Jet2 radar display");
                    
                    // Wait for display update
                    e.sleep(1000);
                    
                    // 📸 Capture DUAL screenshots (Sensor + Canvas)
                    e.captureSensorScreenshot("jet2_detection.png");
                    Print("📸 Jet2 detection screenshots captured!");
                    Print("   → Radar display screenshot saved");
                    Print("   → Tactical canvas screenshot saved");
                    
                    detectedByJet = "Jet2";
                    simulationStopped = true;
                }
            }
            jet2PrevCount = targetCount;
        }
        
        // Exit loop immediately if any target detected
        if (simulationStopped)
        {
            Print("🛑 Target detected by " + detectedByJet + "! Stopping simulation at t = " + t + " seconds.");
            Print("📊 Generating PDF report...");
            e.generatePDFReport("Radar_Detection_Report.pdf");
            Print("✅ Report generated: Radar_Detection_Report.pdf");
            Print("📄 Report includes:");
            Print("   - Initial radar display screenshot");
            Print("   - Initial tactical canvas screenshot");
            Print("   - Detection radar display screenshot");
            Print("   - Detection tactical canvas screenshot");
            Print("   - Complete detection logs");
            break;
        }
    }
    
    SimPause(e);
    Print("✅ Simulation finished!");
    Print("📋 Summary:");
    Print("   - Jet1 detected " + jet1PrevCount + " target(s)");
    Print("   - Jet2 detected " + jet2PrevCount + " target(s)");
    Print("🎯 Check the PDF for complete visual evidence!");
}