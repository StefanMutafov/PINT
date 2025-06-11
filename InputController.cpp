#include "InputController.h"

#include <Arduino.h>

#define L_BUT_PIN 0
#define R_BUT_PIN 35

InputController::InputController()
  : m_prev_L {0}
  , m_prev_R {0}
  , m_last_Ldebounce{0}
  , m_last_Rdebounce{0}
{
  
}

void InputController::init()
{
  pinMode(L_BUT_PIN, INPUT_PULLUP);
  pinMode(R_BUT_PIN, INPUT_PULLUP);
  m_prev_L = digitalRead(L_BUT_PIN);
  m_prev_R = digitalRead(R_BUT_PIN);
}

bool InputController::L_pressed()
{
  int current = digitalRead(L_BUT_PIN);
  static int debounced = HIGH;

  if (current != m_prev_L)
  {
    m_last_Ldebounce = millis();
  }

  if ((millis() - m_last_Ldebounce > debounce_time))
  {
    if (current != debounced) {
      debounced = current;

      if (debounced == LOW)
      {
        m_prev_L = current;
        return true;
      }
    }
  }
  m_prev_L = current;
  return false;
}

bool InputController::R_pressed()
{
  int current = digitalRead(R_BUT_PIN);
  static int debounced = HIGH;

  if (current != m_prev_R)
  {
    m_last_Rdebounce = millis();
  }

  if ((millis() - m_last_Rdebounce > debounce_time))
  {
    if (current != debounced) {
      debounced = current;

      if (debounced == LOW)
      {
        m_prev_R = current;
        return true;
      }
    }
  }
  m_prev_R = current;
  return false;
}
