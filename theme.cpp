#include "theme.h"

Theme makeTheme(bool dark)
{
    Theme t;
    t.dark = dark;
    if (!dark) {
        t.bg = "#f8f6f2"; t.card = "#ffffff"; t.cardAlt = "#f3efe7"; t.line = "#e8e4db";
        t.orange = "#f2600c"; t.orangeHover = "#d95305"; t.orangeSoft = "#fdeee2";
        t.text = "#1f1b16"; t.muted = "#6e675d"; t.dim = "#9c9588";
        t.ok = "#1a7f4b"; t.bad = "#c0392b"; t.warn = "#a66a00";
    } else {
        t.bg = "#12100c"; t.card = "#1c1914"; t.cardAlt = "#14120e"; t.line = "#2f2a22";
        t.orange = "#ff7a2e"; t.orangeHover = "#ff9150"; t.orangeSoft = "rgba(255,122,46,36)";
        t.text = "#f3eee4"; t.muted = "#b5ac9d"; t.dim = "#8c8374";
        t.ok = "#4cd08a"; t.bad = "#ff7b6b"; t.warn = "#f5a524";
    }
    return t;
}

QString buildStyleSheet(const Theme& t)
{
    QString q = QStringLiteral(R"(
QMainWindow, QWidget#body, QWidget#dockbody { background:@bg@; }
QToolTip { background:@card@; color:@text@; border:1px solid @line@; padding:4px 8px; }
QLabel { color:@muted@; background:transparent; }
QLabel#h2 { font-family:"JetBrains Mono"; font-weight:700; font-size:8pt; color:@dim@; }
QLabel#hint { color:@dim@; font-size:9pt; }
QLabel#mono { font-family:"JetBrains Mono"; font-weight:600; font-size:9pt; color:@muted@; }
QLabel#warn { color:@warn@; font-weight:700; }
QLabel#msg { color:@muted@; }
QLabel#msg[err="true"] { color:@bad@; }

QFrame#nav { background:@bg@; border:0; border-bottom:1px solid @line@; }
QLabel#logo { font-family:"JetBrains Mono"; font-weight:800; font-size:14pt; color:@text@; }
QFrame#pill { background:@orangeSoft@; border:0; border-radius:14px; }
QLabel#state { color:@orange@; font-weight:700; font-size:9pt; }
QLabel#dot { background:@dim@; border-radius:3px; min-width:6px; max-width:6px; min-height:6px; max-height:6px; }
QFrame#pill[state="ok"] QLabel#state { color:@ok@; }
QFrame#pill[state="ok"] QLabel#dot { background:@ok@; }
QFrame#pill[state="wait"] QLabel#state { color:@warn@; }
QFrame#pill[state="wait"] QLabel#dot { background:@warn@; }

QFrame#card { background:@card@; border:1px solid @line@; border-radius:16px; }

QPushButton { background:@cardAlt@; color:@muted@; border:1px solid @line@; border-radius:15px;
              padding:5px 14px; font-weight:600; min-height:20px; }
QPushButton:hover:!disabled { border-color:@orange@; color:@text@; }
QPushButton:checked { background:@orange@; color:#ffffff; border-color:@orange@; }
QPushButton:disabled { color:@dim@; }
QPushButton#primary { background:@orange@; color:#ffffff; border:0; font-weight:700; padding:7px 22px;
                      border-radius:17px; min-height:20px; }
QPushButton#primary:hover:!disabled { background:@orangeHover@; color:#ffffff; }
QPushButton#primary[active="true"] { background:@bad@; }
QPushButton#iconbtn { background:@card@; border:1px solid @line@; border-radius:17px; padding:0;
                      min-width:34px; max-width:34px; min-height:34px; max-height:34px; font-size:13pt; color:@text@; }
QPushButton#iconbtn:hover { border-color:@orange@; }

QLineEdit, QComboBox { background:@cardAlt@; color:@text@; border:1px solid @line@; border-radius:8px;
                       padding:4px 8px; font-family:"JetBrains Mono"; font-weight:600; min-height:20px;
                       selection-background-color:@orange@; selection-color:#ffffff; }
QLineEdit:focus, QComboBox:focus, QComboBox:on { border-color:@orange@; }
QLineEdit:disabled, QComboBox:disabled { color:@dim@; }
QLineEdit[bad="true"] { border-color:@bad@; }
QComboBox::drop-down { border:0; width:22px; }
QComboBox::down-arrow { image:none; width:0; height:0; border-left:4px solid transparent;
                        border-right:4px solid transparent; border-top:5px solid @dim@; margin-right:8px; }
QComboBox QAbstractItemView { background:@card@; color:@text@; border:1px solid @line@; outline:0; padding:4px;
                              selection-background-color:@orangeSoft@; selection-color:@orange@; }

QCheckBox { color:@muted@; spacing:6px; background:transparent; }
QCheckBox:disabled { color:@dim@; }
QCheckBox::indicator { width:15px; height:15px; border:1px solid @line@; border-radius:4px; background:@cardAlt@; }
QCheckBox::indicator:hover { border-color:@orange@; }
QCheckBox::indicator:checked { background:@orange@; border-color:@orange@; image:url(:/check.svg); }

QPlainTextEdit#term { background:#0c1117; color:#d9e1ea; border:1px solid @line@; border-radius:12px;
                      padding:6px 8px; font-family:"JetBrains Mono";
                      selection-background-color:#7a3a12; selection-color:#ffffff; }
QPlainTextEdit#term QScrollBar:vertical { background:transparent; width:10px; margin:3px; }
QPlainTextEdit#term QScrollBar::handle:vertical { background:#3d4c5c; border-radius:3px; min-height:30px; }
QPlainTextEdit#term QScrollBar::handle:vertical:hover { background:#566a7f; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background:transparent; }

QMainWindow::separator { background:@line@; width:1px; height:1px; }
QDockWidget { color:@dim@; font-family:"JetBrains Mono"; font-weight:700; font-size:8pt; }
QDockWidget::title { background:@bg@; padding:8px 14px; border-bottom:1px solid @line@; text-align:left; }
)");
    const QPair<const char*, const QString*> map[] = {
        {"@bg@", &t.bg}, {"@card@", &t.card}, {"@cardAlt@", &t.cardAlt}, {"@line@", &t.line},
        {"@orangeHover@", &t.orangeHover}, {"@orangeSoft@", &t.orangeSoft}, {"@orange@", &t.orange},
        {"@text@", &t.text}, {"@muted@", &t.muted}, {"@dim@", &t.dim},
        {"@ok@", &t.ok}, {"@bad@", &t.bad}, {"@warn@", &t.warn}};
    for (const auto& m : map) q.replace(QLatin1String(m.first), *m.second);
    return q;
}
