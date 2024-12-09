#include <FastLED.h>
#include <Ultrasonic.h>

#define LED_TYPE WS2812               // тип используемых светодиодов (WS2812B)
#define COLOR_ORDER GRB               // порядок цветов (зеленый, красный, синий)
#define LED_PIN D4                    // пин для подключения светодиодной ленты
#define NUM_LEDS 1190                 // общее количество светодиодов в ленте
#define MAX_BRIGHTNESS 210            // максимальная яркость светодиодов (0-255)
#define WHITE_COLOR_BRIGHTNESS  107   // яркость для белого цвета (ограниченная для экономии мощности)
#define EDGE_BRIGHTNESS 25            // минимальная яркость для первого и последнего ступени

#define TRIG_PIN_BOTTOM D5            // триггерный пин для нижнего ультразвукового датчика
#define ECHO_PIN_BOTTOM D6            // эхо-пин для нижнего ультразвукового датчика
#define TRIG_PIN_TOP D7               // триггерный пин для верхнего ультразвукового датчика
#define ECHO_PIN_TOP D8               // эхо-пин для верхнего ультразвукового датчика

#define NUM_STEPS 17                  // количество ступеней на лестнице
#define LEDS_PER_STEP (NUM_LEDS / NUM_STEPS) // количество светодиодов на одну ступень

#define COLOR_BUTTON_PIN D1           // пин для кнопки переключения палитры цветов
#define MODE_BUTTON_PIN D3            // пин для кнопки переключения режимов работы
#define DEBOUNCE_DELAY 250            // задержка для защиты от дребезга кнопки (в миллисекундах)

#define FADE_IN_STEPS_ON 6            // количество шагов для плавного включения светодиодов
#define FADE_OUT_STEPS_OFF 6          // количество шагов для плавного выключения светодиодов
#define FADE_DELAY_ON 2               // задержка между шагами для включения (в миллисекундах)
#define FADE_DELAY_OFF 2              // задержка между шагами для выключения (в миллисекундах)

Ultrasonic ultrasonicBottom(TRIG_PIN_BOTTOM, ECHO_PIN_BOTTOM);  // нижний ультразвуковой датчик
Ultrasonic ultrasonicTop(TRIG_PIN_TOP, ECHO_PIN_TOP);           // верхний ультразвуковой датчик

CRGB leds[NUM_LEDS];               // массив для управления каждым светодиодом
enum LedState {OFF, ON, DIMMED};   // состояния светодиодов: выкл, вкл, затемнено
LedState ledState[NUM_LEDS];       // массив для хранения состояния каждого светодиода

enum AnimationState { IDLE, ANIMATING, HOLDING, TURNING_OFF };  // состояния анимации: в ожидании, анимация, удержание, выключение
AnimationState currentState = IDLE;  // текущее состояние анимации (по умолчанию ожидание)

CRGBPalette16 currentPalette;       // текущая палитра цветов
TBlendType currentBlending = LINEARBLEND; // тип смешивания цветов (линейное смешивание)
uint8_t startIndex = 0;             // индекс для сдвига палитры (используется для "движения" цветов)

bool motionDetectedBottom = false;  // флаг обнаружения движения внизу
bool motionDetectedTop = false;     // флаг обнаружения движения наверху
bool bottomToTop = true;            // направление движения анимации (снизу вверх)

const int DISTANCE_THRESHOLD = 105; // пороговое расстояние для срабатывания датчиков движения (в сантиметрах)

uint8_t currentMode = 0;            // текущий режим работы светодиодов (например, режим палитры)
uint8_t currentOperation = 0;       // текущий режим работы анимации (например, разные временные настройки)

unsigned long lastButtonPressTime = 0;            // время последнего нажатия на кнопку смены палитры
unsigned long lastButtonPressForModeTime = 0;     // время последнего нажатия на кнопку смены режимов
unsigned long lastMotionTime = 0;                 // время последнего обнаруженного движения
unsigned long delayBeforeTurnOff = 0;             // время, которое светодиоды будут гореть перед выключением
unsigned long holdTime = 0;                       // время, на которое свет остаётся включенным после срабатывания датчика
const unsigned long delayBeforeTurnOff_0 = 7000;  // время свечения для первого режима (в миллисекундах)
const unsigned long holdTime_0 = 7000;            // время удержания для первого режима (в миллисекундах)
const unsigned long delayBeforeTurnOff_1 = 10000; // время свечения для второго режима (в миллисекундах)
const unsigned long holdTime_1 = 4000;            // время удержания для второго режима (в миллисекундах)

void setup() {
  delay(2000);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  pinMode(COLOR_BUTTON_PIN, INPUT_PULLUP);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);

  currentPalette = RainbowColors_p;  // Начнем с радужного режима

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
}

void loop() {
  motionDetectedBottom = detectMotion(ultrasonicBottom);
  motionDetectedTop = detectMotion(ultrasonicTop);

  unsigned long currentTime = millis();

  handleOperationSwitch();
  handleButtonPress();

  // Настройка времени для holdTime и delayBeforeTurnOff в зависимости от текущего режима работы
  if (currentOperation == 0) {
    delayBeforeTurnOff = delayBeforeTurnOff_0;
    holdTime = holdTime_0;
  } else if (currentOperation == 1) {
    delayBeforeTurnOff = delayBeforeTurnOff_1;
    holdTime = holdTime_1;
  }

  if (currentOperation == 2) {
    startIndex = startIndex + 1;
    FillLEDsFromPaletteColors(startIndex);  // Заполняем палитрой все светодиоды
    FastLED.show();
    FastLED.delay(1000 / 200);
  }

  // Перелив цветов по палитре всегда работает
  if (currentState == ANIMATING || currentState == HOLDING || currentState == TURNING_OFF) {
    if (currentOperation == 1) {
      startIndex = startIndex + 1;
      FillLEDsFromPaletteColors(startIndex);  // Заполняем палитрой все светодиоды
      FastLED.show();
    }
  }

  if (currentOperation != 2) {
    if (motionDetectedBottom || motionDetectedTop) {
      lastMotionTime = currentTime;
      if (currentState == IDLE) {
        currentState = ANIMATING;
      } else if (currentState == HOLDING) {
        // Если датчик срабатывает во время задержки, продлеваем время свечения
        lastMotionTime = currentTime;
      }
    }

    switch (currentState) {
      case IDLE:
        setFirstAndLastStepToMinimumBrightness();  // Включаем белый цвет на первой и последней ступеньке
        break;

      case ANIMATING:
      
        // Включаем светодиоды с плавным эффектом
        if (currentOperation == 0) {
          if (motionDetectedBottom) {
            bottomToTop = true;
            animateStairsOn(true);
          } else {
            bottomToTop = false;
            animateStairsOn(false);
          }
        } else if (currentOperation == 1) {
          if (motionDetectedBottom || motionDetectedTop) {
            animateStairsFadeIn();
          }
        }
        lastMotionTime = currentTime;
        currentState = HOLDING;
        break;

      case HOLDING:
        if (currentTime - lastMotionTime < holdTime) {
          // Держим свет включенным
        } else {
          currentState = TURNING_OFF;
          lastMotionTime = currentTime;
        }
        break;

      case TURNING_OFF:
        if (currentTime - lastMotionTime >= delayBeforeTurnOff) {
          if (currentOperation == 0) {
            animateStairsOff(bottomToTop);
          } else if (currentOperation == 1) {
            animateStairsFadeOut();
          }
          currentState = IDLE;
        }
        break;
    }
  }
}

DEFINE_GRADIENT_PALETTE(blue_palette) {
  0,     0,  0, 255,   // Синий
  255,   0,  0, 255    // Синий
};

DEFINE_GRADIENT_PALETTE(white_palette) {
  0,   255, 255, 255,  // Белый
  255, 255, 255, 255   // Белый
};

DEFINE_GRADIENT_PALETTE(warm_white_palette) {
  0,   255, 150, 0,  // Теплый белый
  255, 255, 150, 0   // Теплый белый
};

void setFirstAndLastStepToMinimumBrightness() {
  // Очищаем все светодиоды перед тем, как установить первые и последние
  FastLED.clear();

  // Включаем белый цвет для первой ступени
  for (int i = 0; i < LEDS_PER_STEP; i++) {
    leds[i] = CRGB::White;
    if (currentOperation == 1) {
      FastLED.setBrightness(EDGE_BRIGHTNESS);
    } else {
      leds[i].nscale8_video(EDGE_BRIGHTNESS); 
    }
  }

  // Включаем белый цвет для последней ступени
  for (int i = NUM_LEDS - LEDS_PER_STEP; i < NUM_LEDS; i++) {
    leds[i] = CRGB::White;
    if (currentOperation == 1) {
      FastLED.setBrightness(EDGE_BRIGHTNESS);
    } else {
      leds[i].nscale8_video(EDGE_BRIGHTNESS); 
    }
  }

  // Применяем изменения к светодиодам
  FastLED.show();
}

// Функция для детекции движения, возвращает максимальное из шести измерений
bool detectMotion(Ultrasonic &sensor) {
  // Первое измерение
  int distance1_1 = sensor.read();
  delay(8);
  int distance1_2 = sensor.read();
  delay(8);
  int distance1_3 = sensor.read();

  // Находим максимальное значение первого измерения
  int maxDistance1 = max(distance1_1, max(distance1_2, distance1_3));

  // Если максимальное значение первого измерения меньше порога, делаем вторую проверку
  if (maxDistance1 < DISTANCE_THRESHOLD) {
    // Второе измерение
    int distance2_1 = sensor.read();
    delay(8);
    int distance2_2 = sensor.read();
    delay(8);
    int distance2_3 = sensor.read();

    // Находим максимальное значение второго измерения
    int maxDistance2 = max(distance2_1, max(distance2_2, distance2_3));

    // Если и второе измерение меньше порога, делаем третью проверку
    if (maxDistance2 < DISTANCE_THRESHOLD) {
      // Третье измерение
      int distance3_1 = sensor.read();
      delay(8);
      int distance3_2 = sensor.read();

      // Находим максимальное значение третьего измерения
      int maxDistance3 = max(distance3_1, distance3_2);
      
      // Если все 3 измерения показывают меньше порогового значения, считаем, что сработало движение
      if (maxDistance3 < DISTANCE_THRESHOLD) {
        return true;  // Датчик сработал
      }
    }
  }

  // Если одно из измерений показывает больше порога, возвращаем false
  return false;
}

// Функция для получения максимальной яркости белого цвета и палитры CloudColors_p
int getMaxBrightness() {
  if (currentPalette == white_palette) {
    return WHITE_COLOR_BRIGHTNESS;  // Яркость для белого
  } else if (currentPalette == CloudColors_p) {
    return 200;  // Яркость для облачных цветов (CloudColors_p = 160 для 150Вт)
  } else {
    return MAX_BRIGHTNESS;  // Яркость для остальных палитр
  }
}

// Обработка нажатия кнопки
void handleButtonPress() {
  if (digitalRead(COLOR_BUTTON_PIN) == LOW && (millis() - lastButtonPressTime) > DEBOUNCE_DELAY) {
    lastButtonPressTime = millis();
    currentMode = (currentMode + 1) % 8;  // Переключаем режимы

    switch (currentMode) {
      case 0:
        currentPalette = RainbowColors_p;
        // Serial.println("Mode 0: Rainbow Colors");
        break;
      case 1:
        currentPalette = RainbowStripeColors_p;
        // Serial.println("Mode 1: Rainbow Stripe Colors");
        break;
      case 2:
        currentPalette = CloudColors_p;
        // Serial.println("Mode 4: Cloud Colors");
        break;
      case 3:
        currentPalette = PartyColors_p;
        // Serial.println("Mode 2: Party Colors");
        break;
      case 4:
        currentPalette = OceanColors_p;
        // Serial.println("Mode 3: Ocean Colors");
        break;
      case 5:
        currentPalette = blue_palette;
        // Serial.println("Mode 5: Solid Blue");
        break;
      case 6:
        currentPalette = white_palette;
        // Serial.println("Mode 6: Solid White");
        break;
      case 7:
        currentPalette = warm_white_palette;
        // Serial.println("Mode 7: Warm White");
        break;
    }

    FastLED.setBrightness(getMaxBrightness()); // Устанавливаем максимальную яркость на основе текущей палитры

    FastLED.show();  // Применяем изменения яркости и палитры
  }
}

// Функция плавного включения светодиодов
void animateStairsFadeIn() {
  FastLED.clear();
  FastLED.setBrightness(getMaxBrightness());  // Плавно увеличиваем яркость
  FastLED.show();  // Применяем изменения
  delay(10);  // Небольшая задержка для плавности
}

// Функция плавного выключения светодиодов
void animateStairsFadeOut() {
  int maxBrightness = getMaxBrightness();  // Используем функцию для получения максимальной яркости
  
  for (int fadeStep = maxBrightness; fadeStep >= 0; fadeStep -= (maxBrightness / 10)) {
    FastLED.setBrightness(fadeStep);
    FastLED.show();
    delay(5);
  }
}

// Заполняем светодиоды цветами из палитры, в зависимости от индекса
void FillLEDsForAmimate(uint8_t colorIndex, int ledIndex) {
  uint8_t BrightnessForPalette = getMaxBrightness();  // Используем динамическую яркость
  leds[ledIndex] = ColorFromPalette(currentPalette, colorIndex, BrightnessForPalette, currentBlending);
}

// Заполняем все светодиоды цветами из палитры
void FillLEDsFromPaletteColors(uint8_t colorIndex) {
  uint8_t BrightnessForPalette = getMaxBrightness();  // Используем динамическую яркость
  for (int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, BrightnessForPalette, currentBlending);
    colorIndex += 3;
  }
}

void handleOperationSwitch() {
  if (digitalRead(MODE_BUTTON_PIN) == LOW && (millis() - lastButtonPressForModeTime) > DEBOUNCE_DELAY) {
    lastButtonPressForModeTime = millis();
    currentOperation = (currentOperation + 1) % 3;  // Переключаем между режимами работы
    switch (currentOperation) {
      case 0:
        bottomToTop = true;
        currentState = TURNING_OFF;
        // Serial.println("currentOperation 0: Handle Motion Detection");
        break;
      case 1:
        FastLED.clear();
        currentState = ANIMATING;
        FastLED.show();
        // Serial.println("currentOperation 1: Handle Motion Color Palette");
        break;
      case 2:
        FastLED.clear();
        FastLED.setBrightness(getMaxBrightness());  // Используем функцию для получения максимальной яркости
        FastLED.show();
        // Serial.println("currentOperation 2: Only Color Palette");
        break;
    }
  }
}

void animateStairsOn(bool bottomToTop) {
  if (bottomToTop) {
    for (int step = 0; step < NUM_STEPS; step++) {
      // Определяем коэффициент для colorIndex в зависимости от текущей палитры
      int colorIndexFactor = 30;  // Значение по умолчанию
      if (currentPalette == RainbowColors_p) {
        colorIndexFactor = 595;
      } else if (currentPalette == RainbowStripeColors_p) {
        colorIndexFactor = 186;
      } else if (currentPalette == CloudColors_p) {
        colorIndexFactor = 175;
      } else if (currentPalette == PartyColors_p) {
        colorIndexFactor = 116;
      } else if (currentPalette == OceanColors_p) {
        colorIndexFactor = 105;
      }

      // Анимация по ступеням
      for (int fadeStep = 0; fadeStep < FADE_IN_STEPS_ON; fadeStep++) {
        for (int i = 0; i < LEDS_PER_STEP; i++) {
          int ledIndex = step * LEDS_PER_STEP + i;
          if (ledState[ledIndex] == OFF) { 
            // Рассчитываем colorIndex на основе коэффициента
            uint8_t colorIndex = startIndex + ledIndex * (595 / colorIndexFactor);

            // Задаём цвет и плавное увеличение яркости
            FillLEDsForAmimate(colorIndex, ledIndex);
            leds[ledIndex].nscale8((fadeStep * 255) / FADE_IN_STEPS_ON);
          }
        }
        FastLED.show();
        delay(FADE_DELAY_ON); 
      }

      // Устанавливаем состояние светодиодов как "Включено"
      for (int i = 0; i < LEDS_PER_STEP; i++) {
        int ledIndex = step * LEDS_PER_STEP + i;
        ledState[ledIndex] = ON;
      }
      FastLED.show();
    }
  } else {
    for (int step = NUM_STEPS - 1; step >= 0; step--) {
      // Определяем коэффициент для colorIndex в зависимости от текущей палитры
      int colorIndexFactor = 30;  // Значение по умолчанию
      if (currentPalette == RainbowColors_p) {
        colorIndexFactor = 595;
      } else if (currentPalette == RainbowStripeColors_p) {
        colorIndexFactor = 186;
      } else if (currentPalette == CloudColors_p) {
        colorIndexFactor = 175;
      } else if (currentPalette == PartyColors_p) {
        colorIndexFactor = 116;
      } else if (currentPalette == OceanColors_p) {
        colorIndexFactor = 105;
      }

      for (int fadeStep = 0; fadeStep < FADE_IN_STEPS_ON; fadeStep++) {
        for (int i = 0; i < LEDS_PER_STEP; i++) {
          int ledIndex = step * LEDS_PER_STEP + i;
          if (ledState[ledIndex] == OFF) { 
            // Задаём цвет в зависимости от текущего режима
            uint8_t colorIndex = startIndex + ledIndex * (595 / colorIndexFactor); // Расчет цвета на основе индекса светодиода
            FillLEDsForAmimate(colorIndex, ledIndex);  // Заполняем светодиоды цветом 
            // Плавное увеличение яркости
            leds[ledIndex].nscale8((fadeStep * 255) / FADE_IN_STEPS_ON);
          }
        }
        FastLED.show();
        delay(FADE_DELAY_ON); 
      }

      for (int i = 0; i < LEDS_PER_STEP; i++) {
        int ledIndex = step * LEDS_PER_STEP + i;
        ledState[ledIndex] = ON;
      }
      FastLED.show();
    }
  }
}

void animateStairsOff(bool bottomToTop) {
  if (bottomToTop) {
    // Проходим по каждой ступеньке снизу вверх
    for (int step = 0; step < NUM_STEPS; step++) {
      if (detectMotion(ultrasonicBottom) || detectMotion(ultrasonicTop)) {
        lastMotionTime = millis();  // Обновляем время последнего движения
        currentState = ANIMATING;     // Возвращаемся в состояние ANIMATING
        return;
      }
      
      // Плавное уменьшение яркости светодиодов на ступеньке
      for (int fadeStep = 0; fadeStep <= FADE_OUT_STEPS_OFF; fadeStep++) {
        float brightnessFactor = 1.0 - (float)fadeStep / (float)FADE_OUT_STEPS_OFF; // Коэффициент уменьшения яркости
        for (int i = 0; i < LEDS_PER_STEP; i++) {
          int ledIndex = step * LEDS_PER_STEP + i;
          leds[ledIndex] = leds[ledIndex].nscale8(brightnessFactor * 255); // Применяем уменьшение яркости
        }
        FastLED.show();
        delay(FADE_DELAY_OFF); // Задержка между шагами
      }
      
      // После завершения анимации выключения этой ступеньки устанавливаем состояние светодиодов как выключенные
      for (int i = 0; i < LEDS_PER_STEP; i++) {
        leds[step * LEDS_PER_STEP + i] = CRGB::Black; // Полное выключение светодиодов
        ledState[step * LEDS_PER_STEP + i] = OFF;
      }
    }
  } else {
    // Проходим по каждой ступеньке сверху вниз
    for (int step = NUM_STEPS - 1; step >= 0; step--) {
      if (detectMotion(ultrasonicBottom) || detectMotion(ultrasonicTop)) {
        lastMotionTime = millis();  // Обновляем время последнего движения
        currentState = ANIMATING;     // Возвращаемся в состояние ANIMATING
        return;
      }
      
      // Плавное уменьшение яркости светодиодов на ступеньке
      for (int fadeStep = 0; fadeStep <= FADE_OUT_STEPS_OFF; fadeStep++) {
        float brightnessFactor = 1.0 - (float)fadeStep / (float)FADE_OUT_STEPS_OFF; // Коэффициент уменьшения яркости
        for (int i = 0; i < LEDS_PER_STEP; i++) {
          int ledIndex = step * LEDS_PER_STEP + i;
          leds[ledIndex] = leds[ledIndex].nscale8(brightnessFactor * 255); // Применяем уменьшение яркости
        }
        FastLED.show();
        delay(FADE_DELAY_OFF); // Задержка между шагами
      }
      
      // После завершения анимации выключения этой ступеньки устанавливаем состояние светодиодов как выключенные
      for (int i = 0; i < LEDS_PER_STEP; i++) {
        leds[step * LEDS_PER_STEP + i] = CRGB::Black; // Полное выключение светодиодов
        ledState[step * LEDS_PER_STEP + i] = OFF;
      }
    }
  }
}