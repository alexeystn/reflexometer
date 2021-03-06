# Reflexometer
Arduino-based device to measure human reaction time.  
Устройство для определения скорости реакции человека на Arduino и экране SSD1306.

![Photo](/photo.jpg)

https://www.instagram.com/p/CA3clOQgHg0/

### Режим 1: измерение скорости реакции 
В произвольное время загорается светодиод.  
Пользователь старается как можно быстрее отреагировать и нажать на кнопку.  
На экране показывается время реакции, а также считается среднее за несколько попыток.  
_Для большинства людей это время составляет 200-300 миллисекунд._  

### Режим 2: распознавание интервала между двумя событиями
Два светодиода мигают с частотой 1 Герц, но не синхронно, а с очень малым сдвигом относительно друг друга.  
В каждом эксперименте прибор случайным образом выбирает, какой диод зажигать первым. Временную разницу можно настроить.  
Пользователь пытается определить, который из светодиодов загорается чуть раньше.  
Прибор показывает, правильно ли ответил человек, и выводит статистику по количеству правильных ответов.  
_Обычно чётко различимый интервал составляет 30-40 миллисекунд._  
