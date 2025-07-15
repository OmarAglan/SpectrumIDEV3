#include "AlifComplete.h"
#include "../LspClient/SpectrumLspClient.h"

#include <QVBoxLayout>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QStringList>
#include <QLabel>
#include <QTimer>
#include <QApplication>
#include <QKeyEvent>
#include <QTextCursor>
#include <QRegularExpression>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>


AutoComplete::AutoComplete(QPlainTextEdit* editor, QObject* parent)
    : QObject(parent), editor(editor) {

    // Initialize Arabic completion widget
    m_arabicCompletionWidget = new ArabicCompletionWidget(editor);

    // Connect Arabic completion signals
    connect(m_arabicCompletionWidget, &ArabicCompletionWidget::itemSelected,
            this, &AutoComplete::onArabicCompletionItemSelected);
    connect(m_arabicCompletionWidget, &ArabicCompletionWidget::itemActivated,
            this, &AutoComplete::onArabicCompletionItemActivated);
    connect(m_arabicCompletionWidget, &ArabicCompletionWidget::exampleInsertRequested,
            this, &AutoComplete::onArabicExampleInsertRequested);
    connect(m_arabicCompletionWidget, &ArabicCompletionWidget::cancelled,
            this, &AutoComplete::onArabicCompletionCancelled);

    // Initialize typing delay timer
    m_typingDelayTimer = new QTimer(this);
    m_typingDelayTimer->setSingleShot(true);
    m_typingDelayTimer->setInterval(TYPING_DELAY_MS);
    connect(m_typingDelayTimer, &QTimer::timeout, this, &AutoComplete::onTypingDelayTimeout);
    keywords = QStringList()
               << "اطبع"
               << "اذا"
               << "اواذا"
               << "استمر"
               << "ارجع"
               << "استورد"
               << "احذف"
               << "ادخل"
               << "اصل"
               << "او"
               << "انتظر"

               << "بينما"

               << "توقف"

               << "حاول"

               << "خطأ"
               << "خلل"

               << "دالة"

               << "صنف"
               << "صح"
               << "صحيح"

               << "عدم"
               << "عند"
               << "عام"
               << "عشري"

               << "في"

               << "ك"

               << "لاجل"
               << "ليس"

               << "مرر"
               << "من"
               << "مزامنة"
               << "مدى"
               << "مصفوفة"

               << "نطاق"
               << "نهاية"

               << "هل"

               << "والا"
               << "ولد"
               << "و"

               << "_تهيئة_";

    shortcuts = {
        {"اطبع", "اطبع($1)"},
        {"اذا", "اذا $1:\n\t\nوالا:\n\t"},
        {"اواذا", "اواذا $1:\n\t"},
        {"استمر", "استمر"},
        {"ارجع", "ارجع $1"},
        {"استورد", "استورد $1"},
        {"احذف", "احذف $1"},
        {"ادخل", "ادخل($1)"},
        {"اصل", "اصل()._تهيئة_($1)"},
        {"او", "او"},
        {"انتظر", "انتظر"},
        {"بينما", "بينما $1:\n\t"},
        {"توقف", "توقف"},
        {"حاول", "حاول:\n\t\nخلل:\n\t\nنهاية:\n\t"},
        {"خطأ", "خطأ"},
        {"خلل", "خلل:\n\t"},
        {"دالة", "دالة $1():\n\t"},
        {"صنف", "صنف $1:\n\tدالة _تهيئة_(هذا):\n\t\t"},
        {"صح", "صح"},
        {"صحيح", "صحيح($1)"},
        {"عدم", "عدم"},
        {"عند", "عند $1 ك :\n\t"},
        {"عام", "عام $1"},
        {"عشري", "عشري($1)"},
        {"في", "في"},
        {"ك", "ك"},
        {"لاجل", "لاجل $1 في :\n\t"},
        {"ليس", "ليس"},
        {"مرر", "مرر"},
        {"من", "من $1 استورد "},
        {"مزامنة", "مزامنة"},
        {"مدى", "مدى($1)"},
        {"مصفوفة", "مصفوفة($1)"},
        {"نطاق", "نطاق $1"},
        {"نهاية", "نهاية $1:\n\t"},
        {"هل", "هل"},
        {"والا", "والا:\n\t$1"},
        {"ولد", "ولد $1"},
        {"و", "و"},
        {"_تهيئة_", "دالة _تهيئة_(هذا):\n\t"}
    };        
    descriptions = {
        {"اطبع", "لعرض قيمة في الطرفية."},
        {"اذا", "تنفيذ أمر في حال تحقق الشرط."},
        {"اواذا", "التحقق من شرط إضافي بعد الشرط 'اذا'."},
        {"استمر", "الانتقال إلى التكرار التالي."},
        {"ارجع", "إرجاع قيمة من دالة."},
        {"استورد", "تضمين مكتبة خارجية."},
        {"احذف", "حذف متغير من الذاكرة."},
        {"ادخل", "قراءة مدخل من المستخدم."},
        {"اصل", "تستخدم لتهيئة الصنف الموروث."},
        {"او", "يكفي تحقق أحد الشرطين."},
        {"انتظر", "تتوقف الدالة عن التنفيذ الى حين قدوم النتائج."},
        {"بينما", "حلقة تعمل طالما أن الشرط صحيح."},
        {"توقف", "إيقاف تنفيذ تكرار الحلقة."},
        {"حاول", "محاولة تنفيذ الشفرة فإن ظهر خلل تنتقل إلى تنفيذ مرحلة'خلل'."},
        {"خطأ", "قيمة منطقية تدل على أن الشرط غير محقق."},
        {"خلل", "يتم تنفيذها في حال ظهور خلل ما في مرحلة تنفيذ 'حاول'."},
        {"دالة", "تعريف دالة جديدة تحتوي برنامج يتم تنفيذه عند استدعائها."},
        {"صنف", "إنشاء كائن يمتلك صفات ودوال."},
        {"صح", "قيمة منطقية تدل على أن الشرط محقق."},
        {"صحيح", "دالة ضمنية تقوم بتحويل المعامل الممرر الى عدد صحيح."},
        {"عدم", "قيمة فارغة."},
        {"عند", "تستخدم لفتح ملف خارجي والكتابة والقراءة عليه."},
        {"عام", "إخبار النطاق الداخلي أن هذا المتغير عام."},
        {"عشري", "دالة ضمنية تقوم بتحويل المعامل الممرر الى عدد عشري."},
        {"في", "تقوم بالتحقق ما إذا كانت القيمة ضمن حاوية مثل المصفوفة."},
        {"ك", "تحدد اسم الملف البديل عند فتحه."},
        {"لاجل", "حلقة تكرار ضمن مدى من الاعداد او مجموعة عناصر حاوية كالمصفوفة."},
        {"ليس", "نفي شرط أو قيمة."},
        {"مرر", "لا تقم بعمل شيء."},
        {"من", "تستخدم لاستيراد جزء محدد من ملف كاستيراد دالة واحدة."},
        {"مزامنة", "تجعل الدالة تزامنية بحيث تتوقف لإنتظار النتائج."},
        {"مدى", "تحديد مدى عددي من وإلى والخطوات."},
        {"مصفوفة", "دالة ضمنية تقوم بتحويل المعامل الممرر الى مصفوفة."},
        {"نطاق", "إخبار النطاق الداخلي أن هذا المتغير في نطاق اعلى ولكنه ليس عام."},
        {"نهاية", "يتم تنفيذ هذه الحالة بعد الإنتهاء من حالة 'حاول' مهما كانت النتيجة."},
        {"هل", "تستخدم للتحقق من قيمتين إن كانتا متطابقتين في النوع."},
        {"والا", "في حال عدم تحقق شرط 'اذا' يتم تنفيذها."},
        {"ولد", "تقوم بإرجاع قيم متتالية من دالة."},
        {"و", "أي يجب تحقق الشرطين معًا."},
        {"_تهيئة_", "دالة تقوم بتهيئة الصنف بشكل تلقائي عند استدعائه."},
    };    

    popup = new QWidget(editor, Qt::ToolTip | Qt::FramelessWindowHint);
    popup->setStyleSheet(
        "QWidget { background-color: #242533; color: #cccccc; }"
        "QListWidget { background-color: #242533; color: #cccccc; }"
        "QListWidget::item { padding: 7px; }"
        "QListWidget::item:selected { background-color: #3a3d54; color: #10a8f4; padding: 0 12px 0 0; }");

    QVBoxLayout* popupLayout = new QVBoxLayout(popup);
    popupLayout->setContentsMargins(0, 0, 0, 0);

    listWidget = new QListWidget(popup);

    QLabel* descriptionLabel = new QLabel(popup);
    descriptionLabel->setStyleSheet("color: #cccccc; padding: 3px;");
    descriptionLabel->setWordWrap(true);

    // set layouts
    popupLayout->addWidget(listWidget);
    popupLayout->addWidget(descriptionLabel);
    popup->setLayout(popupLayout);

    // connections
    connect(listWidget, &QListWidget::currentItemChanged, this,
            [=](QListWidgetItem* current, QListWidgetItem* /*previous*/) {
        if (!current) return;
        QString desc = descriptions.value(current->text(), QString());
        if (desc.isEmpty()) {
            return;
        }
        descriptionLabel->setText(desc);
    });
    connect(editor, &QPlainTextEdit::textChanged, this, &AutoComplete::showCompletion);
    connect(listWidget, &QListWidget::itemClicked, this, &AutoComplete::insertCompletion);

    // filters
    editor->installEventFilter(this);
}


bool AutoComplete::eventFilter(QObject* obj, QEvent* event) {
    if (obj == editor and event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        // Handle Arabic completion widget
        if (m_arabicCompletionWidget && m_arabicCompletionWidget->isVisible()) {
            if (keyEvent->key() == Qt::Key_Tab) {
                // Accept inline completion if showing, otherwise activate selected item
                if (m_showingInlineCompletion) {
                    acceptInlineCompletion();
                } else if (m_arabicCompletionWidget->hasSelection()) {
                    onArabicCompletionItemActivated(m_arabicCompletionWidget->getSelectedItem());
                }
                return true;
            } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                if (m_arabicCompletionWidget->hasSelection()) {
                    onArabicCompletionItemActivated(m_arabicCompletionWidget->getSelectedItem());
                }
                return true;
            } else if (keyEvent->key() == Qt::Key_Escape) {
                m_arabicCompletionWidget->hide();
                hideInlineCompletion();
                return true;
            } else if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down) {
                // Forward navigation to Arabic completion widget
                QCoreApplication::sendEvent(m_arabicCompletionWidget, event);
                return true;
            }
        }
        // Handle legacy popup
        else if (popup->isVisible()) {
            if (keyEvent->key() == Qt::Key_Tab
                or keyEvent->key() == Qt::Key_Return
                or keyEvent->key() == Qt::Key_Enter) {
                insertCompletion();
                return true;
            } else if (keyEvent->key() == Qt::Key_Escape) {
                hidePopup();
                return true;
            } else if (keyEvent->key() == Qt::Key_Up
                       or keyEvent->key() == Qt::Key_Down) {
                QCoreApplication::sendEvent(listWidget, event);
                return true;
            } else {
                return false;
            }
        }
        // Handle inline completion when no popup is visible
        else if (m_showingInlineCompletion) {
            if (keyEvent->key() == Qt::Key_Tab) {
                acceptInlineCompletion();
                return true;
            } else if (keyEvent->key() == Qt::Key_Escape) {
                hideInlineCompletion();
                return true;
            }
        }
    } else if (event->type() == QEvent::FocusOut) { // review
        QTimer::singleShot(0, this, [this]() {
            QWidget* newFocus = QApplication::focusWidget();
            if (!newFocus || !popup->isAncestorOf(newFocus)) {
                popup->hide();
            }
        });
    }
    return QObject::eventFilter(obj, event);
}

QString AutoComplete::getCurrentWord() const {
    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    return cursor.selectedText().trimmed();
}

void AutoComplete::showCompletion() {
    // Stop any existing timer and start a new one
    if (m_typingDelayTimer) {
        m_typingDelayTimer->stop();
        m_typingDelayTimer->start();
    }
}

void AutoComplete::onTypingDelayTimeout() {
    QString currentWord = getCurrentWord();
    if (currentWord.isEmpty() or currentWord.length() < 1) {
        hidePopup();
        if (m_arabicCompletionWidget) {
            m_arabicCompletionWidget->hide();
        }
        return;
    }

    // Debug LSP availability
    qDebug() << "AutoComplete: Checking LSP availability...";
    qDebug() << "AutoComplete: m_lspClient:" << (m_lspClient ? "Available" : "NULL");
    if (m_lspClient) {
        qDebug() << "AutoComplete: LSP connected:" << m_lspClient->isConnected();
        qDebug() << "AutoComplete: Connection state:" << static_cast<int>(m_lspClient->getConnectionState());
    }
    qDebug() << "AutoComplete: Waiting for completion:" << m_waitingForLspCompletion;

    // Try LSP completion first if available
    if (m_lspClient && m_lspClient->isConnected() && !m_waitingForLspCompletion) {
        qDebug() << "AutoComplete: ✅ Using LSP completion for word:" << currentWord;

        // Get current cursor position
        QTextCursor cursor = editor->textCursor();
        int line = cursor.blockNumber();  // 0-based line number
        int character = cursor.columnNumber();  // 0-based character position

        // Create document URI (for now, use a placeholder)
        QString uri = "file:///current_document.alif";  // TODO: Use actual file path

        m_waitingForLspCompletion = true;

        // Request LSP completion
        m_lspClient->requestCompletion(
            uri, line, character,
            [this](const QJsonObject& response) {
                this->onLspCompletionReceived(response);
            },
            [this](const QString& error) {
                qWarning() << "AutoComplete: LSP completion error:" << error;
                m_waitingForLspCompletion = false;
                // Fall back to static completion
                showStaticCompletion();
            }
        );

        return;  // Wait for LSP response
    }

    // Fall back to static completion
    qDebug() << "AutoComplete: ❌ Falling back to static completion";
    showStaticCompletion();
}

void AutoComplete::showStaticCompletion() {
    QString currentWord = getCurrentWord();

    qDebug() << "AutoComplete: 📝 Using static completion for word:" << currentWord;

    QStringList suggestions{};
    for (const QString& keyword : keywords) {
        if (keyword.startsWith(currentWord, Qt::CaseInsensitive)) {
            suggestions << keyword;
        }
    }

    if (!suggestions.isEmpty()) {
        listWidget->clear();
        listWidget->addItems(suggestions);
        listWidget->setCurrentRow(0);
        showPopup();
    }
    else {
        hidePopup();
    }
}

void AutoComplete::showPopup() {
    QTextCursor cursor = editor->textCursor();
    QRect rect = editor->cursorRect(cursor);
    QPoint pos = editor->viewport()->mapToGlobal(rect.bottomLeft());

    // Set the minimum width of the popup
    popup->setFixedSize(450, 250);
    int popupWidth = popup->width();
    int popupHeight = popup->height();

    // Adjust the position to the left bottom of the cursor
    pos.setX(pos.x() - popup->width());
    pos.setY(pos.y() + 2); // Add a small offset to avoid overlapping

    // Ensure the popup stays within the screen boundaries
    QScreen* screen = QGuiApplication::screenAt(pos);
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();

        // Adjust X position if the popup goes off the right edge of the screen
        if (pos.x() + popupWidth > screenGeometry.right()) {
            pos.setX(screenGeometry.right() - popupWidth);
        }

        // Adjust Y position if the popup goes off the bottom edge of the screen
        if (pos.y() + popupHeight > screenGeometry.bottom()) {
            pos.setY(screenGeometry.bottom() - popupHeight);
        }
    }

    // Set the popup size and position
    popup->move(pos);
    popup->show();
}

inline void AutoComplete::hidePopup() {
    popup->hide();
}

void AutoComplete::insertCompletion() {
    if (!popup->isVisible()) return;

    QListWidgetItem* item = listWidget->currentItem();
    if (!item) return;

    QString word = item->text();
    if (!shortcuts.contains(word)) return;

    QString text = shortcuts.value(word);
    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();

    placeholderPositions.clear();

    // البحث عن جميع العلامات مثل $1 وغيرها
    QRegularExpression re("\\$(\\d+)");
    QRegularExpressionMatchIterator i = re.globalMatch(text);
    QList<QPair<int, int>> matches;

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        int pos = match.capturedStart();
        int length = match.capturedLength();
        matches.append(qMakePair(pos, length));
    }

    // إزالة العلامات وحساب المواقع الجديدة
    QString newText = text;
    int offset = 0;
    for (const auto &match : matches) {
        int originalPos = match.first - offset;
        int length = match.second;
        newText.remove(originalPos, length);
        placeholderPositions.append(originalPos);
        offset += length;
    }

    cursor.insertText(newText);

    // حفظ المواقع وتحديد المؤشر
    if (!placeholderPositions.isEmpty()) {
        cursor.setPosition(cursor.position() - newText.length() + placeholderPositions.first());
        editor->setTextCursor(cursor);
    } else {
        editor->setTextCursor(cursor);
    }

    hidePopup();
}

void AutoComplete::setLspClient(SpectrumLspClient* lspClient)
{
    m_lspClient = lspClient;
    qDebug() << "AutoComplete: LSP client set, enhanced completion enabled";
}

void AutoComplete::onLspCompletionReceived(const QJsonObject& response)
{
    m_waitingForLspCompletion = false;

    qDebug() << "AutoComplete: Received LSP completion response";

    if (m_useArabicCompletion && m_arabicCompletionWidget) {
        // Use enhanced Arabic completion widget
        QList<ArabicCompletionItem> arabicItems;

        // Parse LSP completion response into Arabic completion items
        QJsonArray items = response.value("items").toArray();

        for (const auto& itemValue : items) {
            QJsonObject itemObj = itemValue.toObject();
            ArabicCompletionItem arabicItem(itemObj);
            arabicItems.append(arabicItem);
        }

        qDebug() << "AutoComplete: Parsed" << arabicItems.size() << "Arabic completion items";

        if (!arabicItems.isEmpty()) {
            // Position the Arabic completion widget
            QTextCursor cursor = editor->textCursor();
            QRect cursorRect = editor->cursorRect(cursor);
            QPoint globalPos = editor->mapToGlobal(cursorRect.bottomLeft());

            // Adjust position to avoid going off screen
            QScreen* screen = QGuiApplication::primaryScreen();
            QRect screenGeometry = screen->availableGeometry();

            if (globalPos.x() + m_arabicCompletionWidget->width() > screenGeometry.right()) {
                globalPos.setX(screenGeometry.right() - m_arabicCompletionWidget->width());
            }
            if (globalPos.y() + m_arabicCompletionWidget->height() > screenGeometry.bottom()) {
                globalPos.setY(globalPos.y() - cursorRect.height() - m_arabicCompletionWidget->height());
            }

            m_arabicCompletionWidget->move(globalPos);
            m_arabicCompletionWidget->showCompletions(arabicItems, getCurrentWord());
        } else {
            // Fall back to static completions if no LSP results
            showCompletion();
        }
    } else {
        // Use legacy completion system
        QStringList lspSuggestions;
        QJsonArray items = response.value("items").toArray();

        for (const auto& itemValue : items) {
            QJsonObject item = itemValue.toObject();
            QString label = item.value("label").toString();
            QString detail = item.value("detail").toString();

            if (!label.isEmpty()) {
                lspSuggestions << label;

                // Store description for this completion item
                if (!detail.isEmpty()) {
                    descriptions[label] = detail;
                }
            }
        }

        qDebug() << "AutoComplete: Parsed" << lspSuggestions.size() << "LSP completions";

        // Show LSP completions if we have any
        if (!lspSuggestions.isEmpty()) {
            listWidget->clear();
            listWidget->addItems(lspSuggestions);
            listWidget->setCurrentRow(0);
            showPopup();
        } else {
            // Fall back to static completions if no LSP results
            showCompletion();
        }
    }
}



bool AutoComplete::isPopupVisible() {
    return popup->isVisible();
}

// Arabic Completion Implementation
void AutoComplete::setArabicCompletionEnabled(bool enabled) {
    m_useArabicCompletion = enabled;

    if (!enabled && m_arabicCompletionWidget) {
        m_arabicCompletionWidget->hide();
    }
}

bool AutoComplete::isArabicCompletionEnabled() const {
    return m_useArabicCompletion;
}

void AutoComplete::onArabicCompletionItemSelected(const ArabicCompletionItem& item) {
    // Show inline completion preview
    QString completionText = item.insertText.isEmpty() ? item.label : item.insertText;
    showInlineCompletion(completionText);
}

void AutoComplete::onArabicCompletionItemActivated(const ArabicCompletionItem& item) {
    // Insert the completion
    if (!editor) return;

    QTextCursor cursor = editor->textCursor();

    // Find the start of the current word
    QString currentWord = getCurrentWord();
    int wordStart = cursor.position() - currentWord.length();

    // Select the current word
    cursor.setPosition(wordStart);
    cursor.setPosition(cursor.position() + currentWord.length(), QTextCursor::KeepAnchor);

    // Insert the completion text
    QString insertText = item.insertText.isEmpty() ? item.label : item.insertText;
    cursor.insertText(insertText);

    // Hide the completion widget
    m_arabicCompletionWidget->hide();

    // Handle snippet expansion if needed
    if (item.kind == 15) { // Snippet
        // TODO: Implement snippet expansion with placeholders
        // For now, just insert the text
    }
}

void AutoComplete::onArabicExampleInsertRequested(const ArabicCompletionItem& item) {
    // Insert the example code instead of just the completion text
    if (!editor) return;

    QTextCursor cursor = editor->textCursor();

    // Get the example text (prefer arabicExample over usageExample)
    QString exampleText = item.arabicExample.isEmpty() ? item.usageExample : item.arabicExample;

    if (exampleText.isEmpty()) {
        // Fall back to regular completion if no example
        onArabicCompletionItemActivated(item);
        return;
    }

    // Clean up the example text (remove any leading/trailing whitespace and comments)
    QStringList lines = exampleText.split('\n');
    QStringList cleanLines;

    for (const QString& line : lines) {
        QString cleanLine = line.trimmed();
        if (!cleanLine.isEmpty() && !cleanLine.startsWith("//")) {
            cleanLines.append(cleanLine);
        }
    }

    if (!cleanLines.isEmpty()) {
        // Insert the first meaningful line of the example
        QString insertText = cleanLines.first();

        // Find the start of the current word
        QString currentWord = getCurrentWord();
        int wordStart = cursor.position() - currentWord.length();

        // Select the current word
        cursor.setPosition(wordStart);
        cursor.setPosition(cursor.position() + currentWord.length(), QTextCursor::KeepAnchor);

        // Insert the example
        cursor.insertText(insertText);
    }

    // Hide the completion widget
    m_arabicCompletionWidget->hide();
}

void AutoComplete::onArabicCompletionCancelled() {
    if (m_arabicCompletionWidget) {
        m_arabicCompletionWidget->hide();
    }
    hideInlineCompletion();
}

// Inline Completion Implementation
void AutoComplete::showInlineCompletion(const QString& completion) {
    if (!editor || completion.isEmpty()) return;

    hideInlineCompletion(); // Clear any existing inline completion

    QString currentWord = getCurrentWord();
    if (currentWord.isEmpty()) return;

    // Check if the completion starts with the current word
    if (!completion.startsWith(currentWord)) return;

    // Get the remaining part to show as gray text
    QString remainingText = completion.mid(currentWord.length());
    if (remainingText.isEmpty()) return;

    // Store the completion info
    m_inlineCompletionText = completion;
    m_inlineCompletionCursor = editor->textCursor();
    m_showingInlineCompletion = true;

    // Create a format for gray text
    QTextCharFormat grayFormat;
    grayFormat.setForeground(QColor(128, 128, 128)); // Gray color
    grayFormat.setBackground(QColor(240, 240, 240, 50)); // Light gray background

    // Insert the gray text
    QTextCursor cursor = editor->textCursor();
    int originalPosition = cursor.position();

    cursor.insertText(remainingText, grayFormat);

    // Move cursor back to original position
    cursor.setPosition(originalPosition);
    editor->setTextCursor(cursor);
}

void AutoComplete::hideInlineCompletion() {
    if (!m_showingInlineCompletion || !editor) return;

    // Remove the gray text by restoring the cursor position
    QTextCursor cursor = editor->textCursor();
    QString currentWord = getCurrentWord();

    // Find and remove any gray text after the current word
    cursor.movePosition(QTextCursor::EndOfWord);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                       m_inlineCompletionText.length() - currentWord.length());

    // Check if the selected text matches our inline completion
    QString selectedText = cursor.selectedText();
    if (selectedText == m_inlineCompletionText.mid(currentWord.length())) {
        cursor.removeSelectedText();
    }

    m_showingInlineCompletion = false;
    m_inlineCompletionText.clear();
}

void AutoComplete::acceptInlineCompletion() {
    if (!m_showingInlineCompletion || !editor) return;

    // Replace current word with the full completion
    QTextCursor cursor = editor->textCursor();
    QString currentWord = getCurrentWord();

    // Find the start of the current word
    int wordStart = cursor.position() - currentWord.length();

    // Select the current word and any gray text
    cursor.setPosition(wordStart);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                       m_inlineCompletionText.length());

    // Insert the completion with normal formatting
    QTextCharFormat normalFormat;
    normalFormat.setForeground(editor->palette().color(QPalette::Text));
    normalFormat.setBackground(QBrush());

    cursor.insertText(m_inlineCompletionText, normalFormat);

    m_showingInlineCompletion = false;
    m_inlineCompletionText.clear();
}
