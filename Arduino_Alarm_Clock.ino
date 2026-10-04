// ספריות הדרושות להפעלת רכיבי המערכת
#include <Wire.h>                 // תקשורת I2C עבור מסך ה-LCD
#include <LiquidCrystal_I2C.h>    // הפעלת מסך LCD באמצעות I2C
#include <ThreeWire.h>            // תקשורת עם רכיב ה-RTC
#include <RtcDS1302.h>            // הפעלת רכיב הזמן DS1302


// יצירת אובייקט עבור מסך LCD בגודל 16x2 ובכתובת I2C 0x27
LiquidCrystal_I2C lcd(0x27, 16, 2);

// הגדרת התקשורת עם רכיב ה-RTC:
// DAT מחובר לפין 10, CLK לפין 9, RST לפין 11
ThreeWire myWire(10, 9, 11);
RtcDS1302<ThreeWire> Rtc(myWire);

// הגדרת הפינים של שלושת כפתורי המשתמש
const int GREEN_BUTTON = 5; // כפתור ירוק - העלאת ערך (UP)
const int BLACK_BUTTON = 6; // כפתור שחור - מעבר בין מצבים ואישור (SET)
const int RED_BUTTON = 7;   // כפתור אדום - הורדת ערך (DOWN)

// הגדרת הפין שאליו מחובר הבאזר
const int BUZZER = 4;

// ערכי ברירת המחדל של השעון המעורר
int alarmHour = 7;
int alarmMinute = 0;

// משתנה הקובע באיזה מצב נמצאת המערכת:
// 0 = תצוגה רגילה
// 1 = הגדרת שעת ההתראה
// 2 = הגדרת דקות ההתראה
int mode = 0;

// משתנה המציין אם השעון המעורר מופעל
bool alarmEnabled = false;

// משתנה המציין אם השעון המעורר מצלצל כרגע
bool alarmRinging = false;

// קובע איזה מסך יוצג במצב הרגיל:
// true = שעה ותאריך
// false = מצב ושעת ההתראה
bool showClockScreen = true;

// שמירת זמן הלחיצה האחרונה על כפתור.
// משמש למניעת קליטה של לחיצה אחת כמספר לחיצות רצופות
unsigned long lastButtonPress = 0;

// שמירת הזמן שבו התחלפה התצוגה לאחרונה
unsigned long lastScreenChange = 0;

// -------------------- אתחול המערכת --------------------
void setup() {

  // פתיחת תקשורת עם ה-Serial Monitor לצורך בדיקה ומעקב
  Serial.begin(9600);

  // הכפתורים מוגדרים כ-INPUT_PULLUP:
  // במצב רגיל נקרא HIGH, ובלחיצה נקרא LOW
  pinMode(GREEN_BUTTON, INPUT_PULLUP);
  pinMode(BLACK_BUTTON, INPUT_PULLUP);
  pinMode(RED_BUTTON, INPUT_PULLUP);

  // הבאזר מוגדר כרכיב פלט
  pinMode(BUZZER, OUTPUT);


  // אתחול מסך ה-LCD והפעלת התאורה האחורית שלו
  lcd.init();
  lcd.backlight();


  // אתחול רכיב ה-RTC
  Rtc.Begin();

  // שמירת תאריך ושעת הקומפילציה לצורך אתחול ה-RTC במקרה הצורך
  RtcDateTime compiled(__DATE__, __TIME__);

  // אם התאריך והשעה השמורים ב-RTC אינם תקינים,
  // הרכיב מאותחל לפי זמן הקומפילציה
  if (!Rtc.IsDateTimeValid())
    Rtc.SetDateTime(compiled);

  // ביטול הגנת כתיבה של ה-RTC במקרה שהיא מופעלת
  if (Rtc.GetIsWriteProtected())
    Rtc.SetIsWriteProtected(false);

  // הפעלת ספירת הזמן במקרה שה-RTC אינו פועל
  if (!Rtc.GetIsRunning())
    Rtc.SetIsRunning(true);

  // ניקוי המסך בסיום האתחול
  lcd.clear();
}

// -------------------- הלולאה הראשית --------------------
void loop() {
  // קריאת התאריך והשעה הנוכחיים מרכיב ה-RTC
  RtcDateTime now = Rtc.GetDateTime();

  // בדיקת לחיצות המשתמש על הכפתורים
  checkButtons();

  // בדיקה האם הגיע הזמן להפעיל את השעון המעורר
  checkAlarm(now);


  // מצב רגיל - כאשר המשתמש אינו מגדיר התראה
  // והשעון המעורר אינו מצלצל
  if (mode == 0 && !alarmRinging) {

    // החלפת המסך המוצג כל 5 שניות
    if (millis() - lastScreenChange >= 5000) {
      lastScreenChange = millis();
      showClockScreen = !showClockScreen;
      lcd.clear();
    }

    // מעבר בין תצוגת השעה והתאריך
    // לבין תצוגת מצב השעון המעורר
    if (showClockScreen)
      showClock(now);
    else
      showAlarmStatus();
  }


  // כאשר המשתמש נמצא במצב הגדרת ההתראה
  else if (mode != 0)
    showAlarmSettings();

  // הצגת נתוני המערכת ב-Serial Monitor לצורך מעקב
  printSerial(now);
  // השהיה קצרה בין מחזורי הלולאה
  delay(100);
}

// -------------------- הצגת שעה ותאריך --------------------
void showClock(const RtcDateTime& now) {

  // שורה ראשונה - תאריך
  lcd.setCursor(0, 0);
  print2(now.Day());
  lcd.print("/");
  print2(now.Month());
  lcd.print("/");
  lcd.print(now.Year());
  lcd.print("      ");

  // שורה שנייה - שעה
  lcd.setCursor(0, 1);
  print2(now.Hour());
  lcd.print(":");
  print2(now.Minute());
  lcd.print(":");
  print2(now.Second());
  lcd.print("        ");
}

// -------------------- הצגת מצב השעון המעורר --------------------
void showAlarmStatus() {

  lcd.setCursor(0, 0);
  // הצגת מצב ההתראה: פעילה או כבויה
  if (alarmEnabled)
    lcd.print("ALARM: ACTIVE   ");
  else
    lcd.print("ALARM: DISABLED ");

  // הצגת השעה שנקבעה להתראה
  lcd.setCursor(0, 1);
  lcd.print("TIME: ");
  print2(alarmHour);
  lcd.print(":");
  print2(alarmMinute);
  lcd.print("     ");
}

// -------------------- מסך הגדרת ההתראה --------------------
void showAlarmSettings() {
  lcd.setCursor(0, 0);
  // הצגת השלב שבו נמצא המשתמש:
  // הגדרת שעות או הגדרת דקות
  if (mode == 1)
    lcd.print("SET ALARM HOUR  ");
  else
    lcd.print("SET ALARM MIN   ");
  lcd.setCursor(0, 1);
  // הצגת שעת ההתראה תוך כדי השינוי
  print2(alarmHour);
  lcd.print(":");
  print2(alarmMinute);
  lcd.print("           ");
}

// -------------------- בדיקת הכפתורים --------------------
void checkButtons() {

  // לחיצה בו-זמנית על UP ו-DOWN מבטלת את ההתראה
  if (digitalRead(GREEN_BUTTON) == LOW &&
      digitalRead(RED_BUTTON) == LOW) {

    // מניעת קליטה חוזרת של אותה לחיצה
    if (millis() - lastButtonPress >= 250) {
      lastButtonPress = millis();

      // כיבוי ההתראה והבאזר
      alarmEnabled = false;
      alarmRinging = false;
      noTone(BUZZER);
      // חזרה למצב התצוגה הרגיל
      mode = 0;
      showClockScreen = true;
      lastScreenChange = millis();

      lcd.clear();
      // הודעה לצורך מעקב ב-Serial Monitor
      Serial.println("UP + DOWN pressed - Alarm DISABLED");
      // המתנה עד לשחרור שני הכפתורים
      while (digitalRead(GREEN_BUTTON) == LOW ||
             digitalRead(RED_BUTTON) == LOW)
        delay(10);
    }
    return;
  }
  // Debounce - התעלמות מלחיצות נוספות במשך 250 אלפיות השנייה
  if (millis() - lastButtonPress < 250)
    return;

  // -------------------- כפתור SET --------------------
  if (digitalRead(BLACK_BUTTON) == LOW) {
    lastButtonPress = millis();
    Serial.println("SET pressed");

    // אם ההתראה מצלצלת, לחיצה על SET מפסיקה אותה
    // וגם מכבה את השעון המעורר
    if (alarmRinging) {
      noTone(BUZZER);
      alarmRinging = false;
      alarmEnabled = false;
      // חזרה למצב הרגיל
      mode = 0;
      showClockScreen = true;
      lastScreenChange = millis();
      lcd.clear();
      Serial.println("Alarm stopped and DISABLED");
      return;
    }
    // מעבר למצב ההגדרה הבא
    mode++;
    // לאחר הגדרת השעה והדקות,
    // לחיצה שלישית על SET שומרת ומפעילה את ההתראה
    if (mode > 2) {
      mode = 0;
      alarmEnabled = true;
      // חזרה למסך הרגיל
      showClockScreen = true;
      lastScreenChange = millis();

      lcd.clear();
      Serial.println("Alarm saved and ACTIVE");
    }
    return;
  }
  // -------------------- כפתור UP --------------------
  if (digitalRead(GREEN_BUTTON) == LOW) {
    lastButtonPress = millis();
    Serial.println("UP pressed");
    // במצב 1 - העלאת השעה
    // לאחר 23 הערך חוזר ל-0
    if (mode == 1)
      alarmHour = (alarmHour + 1) % 24;
    // במצב 2 - העלאת הדקות
    // לאחר 59 הערך חוזר ל-0
    else if (mode == 2)
      alarmMinute = (alarmMinute + 1) % 60;
    return;
  }
  // -------------------- כפתור DOWN --------------------
  if (digitalRead(RED_BUTTON) == LOW) {
    lastButtonPress = millis();
    Serial.println("DOWN pressed");
    // במצב 1 - הורדת השעה
    if (mode == 1) {
      alarmHour--;
      // לפני 0 חוזרים ל-23
      if (alarmHour < 0)
        alarmHour = 23;
    }

    // במצב 2 - הורדת הדקות
    else if (mode == 2) {
      alarmMinute--;
      // לפני 0 חוזרים ל-59
      if (alarmMinute < 0)
        alarmMinute = 59;
    }
  }
}

// -------------------- בדיקת והפעלת ההתראה --------------------
void checkAlarm(const RtcDateTime& now) {
  // שמירת הדקה שבה ההתראה כבר הופעלה,
  // כדי למנוע הפעלה חוזרת מספר פעמים באותה דקה
  static int lastAlarmMinute = -1;


  // ההתראה מופעלת רק כאשר:
  // 1. השעון המעורר פעיל
  // 2. הוא עדיין אינו מצלצל
  // 3. השעה הנוכחית שווה לשעה שהוגדרה
  // 4. הדקה הנוכחית שווה לדקה שהוגדרה
  // 5. ההתראה עדיין לא הופעלה בדקה הנוכחית
  if (alarmEnabled &&
      !alarmRinging &&
      now.Hour() == alarmHour &&
      now.Minute() == alarmMinute &&
      lastAlarmMinute != now.Minute()) {
    alarmRinging = true;
    lastAlarmMinute = now.Minute();
    lcd.clear();
    Serial.println("*** ALARM RINGING ***");
  }

  // כל עוד ההתראה מצלצלת:
  // הפעלת הבאזר והצגת הודעה למשתמש
  if (alarmRinging) {
    tone(BUZZER, 1000);
    lcd.setCursor(0, 0);
    lcd.print("*** ALARM ***   ");
    lcd.setCursor(0, 1);
    lcd.print("PRESS SET       ");
  }
  // כאשר הדקה משתנה ניתן לאפשר הפעלה עתידית מחדש
  if (now.Minute() != alarmMinute) {
    lastAlarmMinute = -1;
  }
}

// -------------------- Serial Monitor --------------------
// פונקציה זו משמשת לצורך בדיקה ומעקב אחר פעילות המערכת
void printSerial(const RtcDateTime& now) {
  static int lastSecond = -1;
  // מניעת הדפסות חוזרות - הנתונים מודפסים פעם אחת בכל שנייה
  if (now.Second() == lastSecond)
    return;
  lastSecond = now.Second();


  // הצגת השעה הנוכחית
  Serial.print("Time: ");
  print2Serial(now.Hour());
  Serial.print(":");
  print2Serial(now.Minute());
  Serial.print(":");
  print2Serial(now.Second());

  // הצגת התאריך הנוכחי
  Serial.print(" | Date: ");

  print2Serial(now.Day());
  Serial.print("/");
  print2Serial(now.Month());
  Serial.print("/");
  Serial.print(now.Year());

  // הצגת שעת ההתראה
  Serial.print(" | Alarm: ");
  print2Serial(alarmHour);
  Serial.print(":");
  print2Serial(alarmMinute);
  // הצגת מצב השעון המעורר
  if (alarmEnabled)
    Serial.println(" ACTIVE");
  else
    Serial.println(" DISABLED");
}
// -------------------- פונקציות עזר --------------------
// הוספת הספרה 0 לפני מספר קטן מ-10 בתצוגת ה-LCD
// לדוגמה: 7 יוצג כ-07
void print2(int number) {
  if (number < 10)
    lcd.print("0");
  lcd.print(number);
}
// אותה פעולה עבור הנתונים המוצגים ב-Serial Monitor
void print2Serial(int number) {
  if (number < 10)
    Serial.print("0");
  Serial.print(number);
}

