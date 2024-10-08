# SmartStairLights

**SmartStairLights** — это проект умной подсветки лестницы с использованием адресной светодиодной ленты WS2812B, управляемой ультразвуковыми датчиками. Система разработана на базе Arduino и написан на языке C++ с использованием **Arduino IDE**. 

## О проекте

Цель проекта — создать "умную" подсветку для лестницы, которая автоматически включается, когда человек начинает подниматься или спускаться. Система использует два ультразвуковых датчика (расположенные вверху и внизу лестницы), которые обнаруживают движение и активируют светодиодную ленту.

Основные возможности проекта:
- Первый режим: подсветка автоматически включается и выключается с помощью ультразвуковых датчиков, реагируя на движение человека в зависимости от направления (вверх или вниз по лестнице). Подсветка плавно и анимационно загорается и гаснет, создавая эффект динамичного освещения каждой ступени.
- Второй режим: подсветка также активируется при обнаружении движения, но в этом режиме сразу включаются все ступени.
- Третий режим: подсветка всегда включена, вне зависимости от показаний датчиков.
- Кнопка для переключения режимов и цветовых эффектов: в системе используется две тактовые кнопки. Первая кнопка переключает режимы, а вторая — переключает цветовые эффекты на светодиодной ленте. Для создания цветовых эффектов используются палитры из библиотеки FastLED, такие как "Цвета радуги" (RainbowColors_p), "Радужные полосы" (RainbowStripeColors_p), "Цвета облаков" (CloudColors_p) и другие.
- Динамическая подсветка: в первом режиме подсветка плавно включается начиная с первой ступени в направлении движения. Система определяет, в какую сторону движется человек (вверх или вниз), и включает подсветку в соответствующем направлении. Также подсветка автоматически отключается через определённое время, если не зафиксировано движение.
```c
unsigned long delayBeforeTurnOff = 0;  // Время свечения после включения всех светодиодов
unsigned long holdTime = 0;            // Время, в течение которого свет остается включенным после обнаружения движения
```
- Постоянная подсветка первой и последней ступени: в режиме с датчиками первые и последние ступени всегда подсвечиваются белым светом на минимальной яркости, чтобы человек мог видеть их даже ночью. Это помогает безопасно определить первую ступеньку в темноте.
```c
#define MIN_BRIGHTNESS 50 // Яркость первой и последней ступени
```

## Аппаратные компоненты

Для реализации проекта использованы следующие компоненты:
- **NodeMCU (ESP8266)**: основной контроллер системы. Можно использовать и другие контроллеры, но важно, чтобы у них было достаточно оперативной памяти. Например, для Arduino UNO недостаточно памяти для управления 1190 светодиодами на адресной ленте.
- **Адресная светодиодная лента (WS2812B)**: лента с плотностью 60 светодиодов на метр, которые могут управляться индивидуально и отображать различные цвета (RGB). В моём случае общая длина ленты составила 19,83 метра, а количество светодиодов — 1190. Лестница состоит из 17 ступеней.
```c
#define NUM_LEDS 1190       // Количество светодиодов
#define NUM_STEPS 17        // Количество ступеней
#define LEDS_PER_STEP (NUM_LEDS / NUM_STEPS) // Количество светодиодов на одной ступени
```
Вы можете изменить эти значения под свою лестницу, но количество светодиодов (NUM_LEDS) должно быть кратным числу ступеней (NUM_STEPS). В моём случае 1190 / 17 = 70 светодиодов на одну ступень.
- **HC-SR04**: ультразвуковые датчики для обнаружения движения. Один датчик устанавливается в нижней части лестницы, а другой — в верхней. В моём случае нижний датчик был установлен примерно на 15 см выше первой ступени, а верхний — на 15 см выше последней ступени. Это позволяет датчикам реагировать на движение ещё до того, как человек наступит на первую или последнюю ступень. Также можно настроить порог срабатывания датчиков в сантиметрах:
```c
const int DISTANCE_THRESHOLD = 105; // Порог срабатывания ультразвуковых датчиков
```
Рекомендуется устанавливать датчики на расстоянии не менее 30 см от ступеней, чтобы избежать ложных срабатываний. В моём случае, при установке датчиков на 10-15 см выше ступеней, иногда происходили ложные срабатывания из-за отражений от самих ступеней. Для решения этой проблемы я сделал тройную проверку функции обнаружения движения (bool detectMotion(Ultrasonic &sensor)), чтобы минимизировать ложные срабатывания.
- **Резисторы, провода и коннекторы**: дополнительные компоненты для подключения и защиты системы. Резистор номиналом 200-500 Ом подключается между пином микроконтроллера и DI-контактом светодиодной ленты. Это предотвращает повреждение ленты, так и пин контроллера в случае, если подается сигнал от микроконтроллера при отсутствии питания на самой ленте. Также рекомендуется использовать провода сечением минимум 1,5 мм² для подключения ленты, особенно на высоких токах.
- **Блок питания 5V 30A (150 Вт)**: для питания светодиодной ленты. При выборе блока питания важно правильно рассчитать мощность. Например, каждый светодиод WS2812B RGB в максимальной яркости (когда все 3 цвета — красный, зеленый и синий — работают на полную) потребляет примерно: 
  Напряжение: 5 В.
  Ток: 50 мА (0.05 А) на один светодиод.
  Мощность: `P = V * I = 5 * 0,05 = 0.25 Вт`.
  Теперь рассчитаем полное потребление для 1190 светодиодов на максимальной яркости:
- `Pmax = 1190 * 0,25 = 297,5 Вт`.
  Я хочу ограничить потребление до 150 Вт. Это значит, что нужно установить яркость таким образом, чтобы мощность на выходе не превышала этого значения. Для этого найдем коэффициент уменьшения яркости:
- `Коэффициент = 150 / 297,5 ≈ 0,50`.
  Это означает, что максимальная яркость должна составлять около 50% от полной яркости. Для белого цвета это примерно:
- `255 * 0,5 ≈ 128`.
  Кроме того, следует учитывать, что блок питания должен составлять 20% от потребляемой мощности. Тогда:
- `128 - 128 * 0,2 ≈ 102`.

```c
#define BRIGHTNESS_FOR_WHITE 102  // Установленная яркость белого цвета на 40%
```
Если используются другие цветовые режимы (например, радуга или один цвет), то потребляемая мощность будет примерно в три раза меньше, что делает блок питания на 150 Вт вполне достаточным.
- **Кнопки**: две тактовые кнопки, которые используются для переключения режимов и цветовых эффектов.

## Схема подключения

Система состоит из двух основных частей:
1. **Ультразвуковые датчики**: один датчик устанавливается в нижней части лестницы, другой — в верхней.
2. **Светодиодная лента**: она размещается вдоль лестницы и разделена на сегменты для каждой ступени.

Полную схему подключения можно найти в разделе "Схемы" (будет добавлено).

### Основные функции:
- `detectMotion(Ultrasonic &sensor)`: функция для определения движения с помощью ультразвукового датчика.
- `animateStairs(bool bottomToTop)`: анимация включения светодиодов по направлению движения.
- `animateStairsOff(bool bottomToTop)`: анимация выключения светодиодов.
- Поддержка различных цветовых режимов: одноцветный режим, режим радужной подсветки и другие.

## Установка

1. Скачайте или клонируйте этот репозиторий:
   ```bash
   git clone https://github.com/AlikhaNatov/SmartStairLights.git
