# 🚦 Traffic Light with Visual Messages - Raspberry Pi Pico W

This project is part of the practical activity for the module **Introduction to Embedded Software Development with Microcontrollers**. The goal is to implement a pedestrian traffic light system using the **Raspberry Pi Pico W** board, displaying informative messages on an **SSD1306 OLED display**.

## 🎯 Objective

Implement a traffic light system that displays messages on the OLED screen according to the current traffic light state:

- 🟢 **GREEN LIGHT - CROSS WITH CAUTION**  
  (When the green LED is active)

- 🟡 **WARNING LIGHT - GET READY**  
  (When the yellow LED is active)

- 🔴 **RED LIGHT - PLEASE WAIT**  
  (When the red LED is active)

## 🧭 Task Steps

1. Use an **OLED display** with the **SSD1306** driver.  
2. Communication with the display should be done via **I2C** protocol.  
3. In the provided code, insert function calls to display messages within:
   - `SinalAberto` (Open Signal)
   - `SinalAtencao` (Warning Signal)
   - `SinalFechado` (Closed Signal)
4. The messages must clearly reflect the current traffic light state.
