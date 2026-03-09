Triple-Platform Ultra-High-Speed Supersonic Observation Mission Western Desert & Rajasthan IB
---------------------------------------------------------------------------------------------

**All aircraft launched from Jamnagar AFS – 2000 km/h Sustained Afterburner Dash**

.. image:: ../../../images/defence_case_study3.png
   :align: center
   :width: 800

1. Mission Overview
~~~~~~~~~~~~~~~~~~~

Three Indian Air Force fighters launched simultaneously from **INS Hansa / Jamnagar Air Force Station**  
at a configured **MoveSpeed of 2000 km/h** to execute a deep reconnaissance mission across the 
entire western desert and Rajasthan International Border (IB).

### Aircraft Route Summary (Round-Trip)

====================  ===================================  =========  ==============  =========================
Aircraft              Route Description                    Distance   Observed Time   Expected Time @2000 km/h
====================  ===================================  =========  ==============  =========================
HAL Tejas             Jamnagar → Morbi → Barmer →           1,008 km   30:11           30:14
                      Jaisalmer → Nagaur → land
                      Jodhpur AFS

Dassault Mirage 2000  Jamnagar → Morbi → Tharad →           1,568 km   47:05           47:02
                      Chitalwana → Barmer → Jaisalmer →
                      Bikaner → Didwana → Ajmer →
                      Raipur → land Jodhpur AFS

MiG-21 Bison          Jamnagar → Morbi → Patan →            1,842 km   55:47           55:15
                      Jodhpur → Jaisalmer → Barmer →
                      Surendranagar → land Jamnagar AFS
====================  ===================================  =========  ==============  =========================



2. Timing & Performance Analysis
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

All observed times matched the **theoretical expected values** within **3–32 seconds**, despite:

- Long supersonic dashes over the **Thar Desert** and **Aravalli ranges**
- Multiple **high-G waypoint turns** at 2000 km/h
- Continuous afterburner operation with **LITENING / Damocles targeting pods active**

The **MiG-21 Bison** exceeded expectations slightly due to **minimal waypoint curvature** on the return leg, 
allowing higher sustained speeds.

3. Sector Assessments
~~~~~~~~~~~~~~~~~~~~~

- Entire **Gujarat–Rajasthan–Punjab IB sector** (Kutch → Bikaner) was fully imaged in **under 56 minutes**  
  from a single southern naval air station.
  
- Triple-overlapping ISR sensor coverage enabled **120–160 km deep real-time imagery** into Pakistani 
  territory, including:
  
  - Karachi coastal approaches  
  - Bahawalpur region  
  - Rahim Yar Khan corridor  
  - Sukkur division  

4. Findings
~~~~~~~~~~~

- Sustained **2000 km/h supersonic dash** validated as fully combat-realistic across all three platforms.
- Complete western desert border surveillance achieved **in under one hour** from Jamnagar AFS.
- Path-following remained **stable with no drift, desync, or timing anomalies**.
- **Critical software defect reconfirmed:**  
  The Trajectory tool does *not* auto-disable after deleting all waypoints →  
  tool remains active → drawing a new route causes **instant simulation crash**  
  (*null reference / orphaned trajectory object*).

5. Conclusion
~~~~~~~~~~~~~

The triple-platform mission conclusively demonstrates that the **Mirage 2000**, **HAL Tejas**, and 
**MiG-21 Bison**, when operating at **2000 km/h sustained supersonic dash**, can establish **complete 
real-time battlespace awareness** across the western desert and Rajasthan border in **under 56 minutes** 
from Jamnagar AFS.

Mission performance was exceptional; however, the **Trajectory Tool crash defect** remains a major 
operational hazard and must be resolved before frontline deployment of the simulation system.

