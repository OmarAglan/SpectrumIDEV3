#include "ArabicCompletionDatabase.h"
#include <algorithm>

namespace als {
namespace features {

// Static member definitions
std::vector<ArabicCompletionItem> ArabicCompletionDatabase::s_allCompletions;
std::vector<CodeSnippet> ArabicCompletionDatabase::s_allSnippets;
std::map<std::string, std::vector<ArabicCompletionItem>> ArabicCompletionDatabase::s_completionsByCategory;
bool ArabicCompletionDatabase::s_initialized = false;

void ArabicCompletionDatabase::initialize() {
    if (s_initialized) return;
    
    // Clear existing data
    s_allCompletions.clear();
    s_allSnippets.clear();
    s_completionsByCategory.clear();
    
    // Load all completion categories
    auto ioCompletions = getIOCompletions();
    auto controlFlowCompletions = getControlFlowCompletions();
    auto dataTypeCompletions = getDataTypeCompletions();
    auto mathCompletions = getMathCompletions();
    auto stringCompletions = getStringCompletions();
    auto arrayCompletions = getArrayCompletions();
    auto functionCompletions = getFunctionCompletions();
    auto classCompletions = getClassCompletions();
    auto errorHandlingCompletions = getErrorHandlingCompletions();
    auto fileIOCompletions = getFileIOCompletions();
    
    // Combine all completions
    s_allCompletions.insert(s_allCompletions.end(), ioCompletions.begin(), ioCompletions.end());
    s_allCompletions.insert(s_allCompletions.end(), controlFlowCompletions.begin(), controlFlowCompletions.end());
    s_allCompletions.insert(s_allCompletions.end(), dataTypeCompletions.begin(), dataTypeCompletions.end());
    s_allCompletions.insert(s_allCompletions.end(), mathCompletions.begin(), mathCompletions.end());
    s_allCompletions.insert(s_allCompletions.end(), stringCompletions.begin(), stringCompletions.end());
    s_allCompletions.insert(s_allCompletions.end(), arrayCompletions.begin(), arrayCompletions.end());
    s_allCompletions.insert(s_allCompletions.end(), functionCompletions.begin(), functionCompletions.end());
    s_allCompletions.insert(s_allCompletions.end(), classCompletions.begin(), classCompletions.end());
    s_allCompletions.insert(s_allCompletions.end(), errorHandlingCompletions.begin(), errorHandlingCompletions.end());
    s_allCompletions.insert(s_allCompletions.end(), fileIOCompletions.begin(), fileIOCompletions.end());
    
    // Organize by category
    s_completionsByCategory["io"] = ioCompletions;
    s_completionsByCategory["control_flow"] = controlFlowCompletions;
    s_completionsByCategory["data_types"] = dataTypeCompletions;
    s_completionsByCategory["math"] = mathCompletions;
    s_completionsByCategory["string"] = stringCompletions;
    s_completionsByCategory["array"] = arrayCompletions;
    s_completionsByCategory["function"] = functionCompletions;
    s_completionsByCategory["class"] = classCompletions;
    s_completionsByCategory["error_handling"] = errorHandlingCompletions;
    s_completionsByCategory["file_io"] = fileIOCompletions;
    
    // Load snippets
    auto controlFlowSnippets = getControlFlowSnippets();
    auto functionSnippets = getFunctionSnippets();
    auto classSnippets = getClassSnippets();
    auto commonPatternSnippets = getCommonPatternSnippets();
    
    s_allSnippets.insert(s_allSnippets.end(), controlFlowSnippets.begin(), controlFlowSnippets.end());
    s_allSnippets.insert(s_allSnippets.end(), functionSnippets.begin(), functionSnippets.end());
    s_allSnippets.insert(s_allSnippets.end(), classSnippets.begin(), classSnippets.end());
    s_allSnippets.insert(s_allSnippets.end(), commonPatternSnippets.begin(), commonPatternSnippets.end());
    
    s_initialized = true;
}

std::vector<ArabicCompletionItem> ArabicCompletionDatabase::getAllCompletions() {
    if (!s_initialized) initialize();
    return s_allCompletions;
}

std::vector<ArabicCompletionItem> ArabicCompletionDatabase::getCompletionsByCategory(const std::string& category) {
    if (!s_initialized) initialize();
    
    auto it = s_completionsByCategory.find(category);
    if (it != s_completionsByCategory.end()) {
        return it->second;
    }
    return {};
}

std::vector<ArabicCompletionItem> ArabicCompletionDatabase::getCompletionsForContext(const std::string& context) {
    if (!s_initialized) initialize();
    
    std::vector<ArabicCompletionItem> result;
    for (const auto& item : s_allCompletions) {
        if (item.isApplicableInContext(context)) {
            result.push_back(item);
        }
    }
    return result;
}

std::vector<CodeSnippet> ArabicCompletionDatabase::getBuiltinSnippets() {
    if (!s_initialized) initialize();
    return s_allSnippets;
}

void ArabicCompletionDatabase::addCustomCompletion(const ArabicCompletionItem& item) {
    if (!s_initialized) initialize();
    s_allCompletions.push_back(item);
    
    // Add to category if specified
    if (!item.category.empty()) {
        s_completionsByCategory[item.category].push_back(item);
    }
}

ArabicCompletionItem* ArabicCompletionDatabase::findCompletion(const std::string& label) {
    if (!s_initialized) initialize();
    
    auto it = std::find_if(s_allCompletions.begin(), s_allCompletions.end(),
        [&label](const ArabicCompletionItem& item) {
            return item.label == label || item.arabicName == label;
        });
    
    return (it != s_allCompletions.end()) ? &(*it) : nullptr;
}

// Helper method implementations
ArabicCompletionItem ArabicCompletionDatabase::createFunction(
    const std::string& arabicName,
    const std::string& englishName,
    const std::string& description,
    const std::string& detailedDesc,
    const std::vector<ParameterInfo>& params,
    const std::string& returnType,
    const std::string& returnDesc,
    int priority) {
    
    ArabicCompletionItem item(arabicName, CompletionItemKind::Function);
    item.arabicName = arabicName;
    item.englishName = englishName;
    item.arabicDescription = description;
    item.arabicDetailedDesc = detailedDesc;
    item.parameters = params;
    item.returnType = returnType;
    item.arabicReturnDesc = returnDesc;
    item.priority = priority;
    item.contexts = {"global", "function", "class"};
    
    // Generate usage example
    std::string usage = arabicName + "(";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) usage += ", ";
        usage += params[i].name;
    }
    usage += ")";
    item.usageExample = usage;
    
    return item;
}

ArabicCompletionItem ArabicCompletionDatabase::createKeyword(
    const std::string& arabicName,
    const std::string& englishName,
    const std::string& description,
    const std::string& detailedDesc,
    const std::string& example,
    int priority) {
    
    ArabicCompletionItem item(arabicName, CompletionItemKind::Keyword);
    item.arabicName = arabicName;
    item.englishName = englishName;
    item.arabicDescription = description;
    item.arabicDetailedDesc = detailedDesc;
    item.arabicExample = example;
    item.priority = priority;
    item.contexts = {"global", "function", "class"};
    
    return item;
}

ParameterInfo ArabicCompletionDatabase::createParam(
    const std::string& name,
    const std::string& type,
    const std::string& description,
    bool optional,
    const std::string& defaultValue) {
    
    ParameterInfo param;
    param.name = name;
    param.type = type;
    param.arabicDescription = description;
    param.isOptional = optional;
    param.defaultValue = defaultValue;
    return param;
}

// I/O Completions
std::vector<ArabicCompletionItem> ArabicCompletionDatabase::getIOCompletions() {
    return {
        createFunction(
            "اطبع", "print",
            "يطبع النص أو القيم المحددة إلى وحدة التحكم",
            "دالة أساسية لطباعة النصوص والقيم. تقبل نص واحد أو أكثر وتطبعهم في سطر واحد مع إضافة سطر جديد في النهاية.",
            {createParam("النص", "نص", "النص أو القيمة المراد طباعتها")},
            "فراغ", "لا ترجع قيمة", 95
        ),

        createFunction(
            "اقرأ", "read",
            "يقرأ نص من المستخدم",
            "دالة لقراءة النص من المستخدم عبر وحدة التحكم. تنتظر حتى يدخل المستخدم النص ويضغط Enter.",
            {createParam("الرسالة", "نص", "رسالة تظهر للمستخدم", true, "\"\"")},
            "نص", "النص الذي أدخله المستخدم", 90
        ),

        createFunction(
            "اقرأ_رقم", "read_number",
            "يقرأ رقم من المستخدم",
            "دالة لقراءة رقم صحيح من المستخدم. تتعامل مع الأخطاء تلقائياً وتطلب من المستخدم إعادة الإدخال إذا لم يكن الإدخال رقماً صحيحاً.",
            {createParam("الرسالة", "نص", "رسالة تظهر للمستخدم", true, "\"\"")},
            "رقم", "الرقم الذي أدخله المستخدم", 85
        ),

        createFunction(
            "اقرأ_رقم_عشري", "read_decimal",
            "يقرأ رقم عشري من المستخدم",
            "دالة لقراءة رقم عشري (فاصلة عائمة) من المستخدم. تتعامل مع الأخطاء تلقائياً.",
            {createParam("الرسالة", "نص", "رسالة تظهر للمستخدم", true, "\"\"")},
            "رقم_عشري", "الرقم العشري الذي أدخله المستخدم", 80
        )
    };
}

// Control Flow Completions
std::vector<ArabicCompletionItem> ArabicCompletionDatabase::getControlFlowCompletions() {
    return {
        createKeyword(
            "اذا", "if",
            "جملة شرطية للتحكم في تدفق البرنامج",
            "تستخدم لتنفيذ كود معين فقط عند تحقق شرط محدد. يمكن استخدامها مع 'اواذا' و 'والا' لإنشاء سلسلة شروط.",
            R"(// شرط بسيط
اذا (العمر >= 18) {
    اطبع("يمكنك التصويت")
}

// شرط مع بديل
اذا (الدرجة >= 60) {
    اطبع("نجحت")
} والا {
    اطبع("راسب")
})",
            90
        ),

        createKeyword(
            "اواذا", "else if",
            "شرط إضافي في سلسلة الشروط",
            "تستخدم لإضافة شرط جديد بعد 'اذا'. يتم فحص الشرط فقط إذا لم تتحقق الشروط السابقة.",
            R"(اذا (الدرجة >= 90) {
    اطبع("ممتاز")
} اواذا (الدرجة >= 80) {
    اطبع("جيد جداً")
} اواذا (الدرجة >= 70) {
    اطبع("جيد")
} والا {
    اطبع("مقبول")
})",
            85
        ),

        createKeyword(
            "والا", "else",
            "البديل الافتراضي في الشروط",
            "تستخدم لتنفيذ كود معين عندما لا تتحقق أي من الشروط السابقة في سلسلة 'اذا'.",
            R"(اذا (الطقس == "مشمس") {
    اطبع("اذهب للنزهة")
} والا {
    اطبع("ابق في المنزل")
})",
            85
        ),

        createKeyword(
            "لكل", "for",
            "حلقة تكرار للعد أو التكرار عبر مجموعة",
            "تستخدم لتكرار تنفيذ كود معين عدد محدد من المرات أو للتكرار عبر عناصر مصفوفة أو قائمة.",
            R"(// حلقة للعد
لكل العداد من 1 إلى 10 {
    اطبع("العدد:", العداد)
}

// حلقة عبر مصفوفة
متغير الأسماء = ["أحمد", "فاطمة", "محمد"]
لكل الاسم في الأسماء {
    اطبع("مرحبا", الاسم)
})",
            88
        )
    };
}

// Data Type Completions
std::vector<ArabicCompletionItem> ArabicCompletionDatabase::getDataTypeCompletions() {
    return {
        createKeyword(
            "متغير", "var",
            "يعرف متغير جديد",
            "كلمة مفتاحية لتعريف متغير جديد. يمكن للمتغير أن يحتوي على أي نوع من البيانات.",
            R"(// تعريف متغيرات مختلفة
متغير الاسم = "أحمد"
متغير العمر = 25
متغير الراتب = 5000.50
متغير متزوج = صحيح)",
            95
        ),

        createKeyword(
            "ثابت", "const",
            "يعرف ثابت لا يمكن تغييره",
            "كلمة مفتاحية لتعريف ثابت. القيمة لا يمكن تغييرها بعد التعريف الأولي.",
            R"(// تعريف ثوابت
ثابت باي = 3.14159
ثابت اسم_البرنامج = "برنامجي"
ثابت الحد_الأقصى = 100)",
            90
        ),

        createKeyword(
            "نص", "string",
            "نوع بيانات للنصوص",
            "نوع بيانات يستخدم لتخزين النصوص والأحرف. يمكن أن يحتوي على أي عدد من الأحرف.",
            R"(نص الرسالة = "مرحبا بالعالم"
نص الاسم_الكامل = الاسم_الأول + " " + الاسم_الأخير)",
            85
        ),

        createKeyword(
            "رقم", "number",
            "نوع بيانات للأرقام الصحيحة",
            "نوع بيانات يستخدم لتخزين الأرقام الصحيحة (بدون فاصلة عشرية).",
            R"(رقم العمر = 25
رقم عدد_الطلاب = 150)",
            85
        ),

        createKeyword(
            "رقم_عشري", "decimal",
            "نوع بيانات للأرقام العشرية",
            "نوع بيانات يستخدم لتخزين الأرقام العشرية (مع فاصلة عشرية).",
            R"(رقم_عشري الراتب = 5000.50
رقم_عشري درجة_الحرارة = 23.5)",
            85
        ),

        createKeyword(
            "منطقي", "boolean",
            "نوع بيانات للقيم المنطقية",
            "نوع بيانات يحتوي على قيمة واحدة من اثنتين: صحيح أو خطأ.",
            R"(منطقي متزوج = صحيح
منطقي مكتمل = خطأ)",
            85
        ),

        createKeyword(
            "صحيح", "true",
            "القيمة المنطقية الصحيحة",
            "قيمة منطقية تمثل الحالة الصحيحة أو الإيجابية.",
            R"(متغير النتيجة = صحيح
اذا (النتيجة == صحيح) {
    اطبع("العملية نجحت")
})",
            80
        ),

        createKeyword(
            "خطأ", "false",
            "القيمة المنطقية الخاطئة",
            "قيمة منطقية تمثل الحالة الخاطئة أو السلبية.",
            R"(متغير مكتمل = خطأ
اذا (مكتمل == خطأ) {
    اطبع("لم تكتمل العملية بعد")
})",
            80
        )
    };
}

// Math Completions
std::vector<ArabicCompletionItem> ArabicCompletionDatabase::getMathCompletions() {
    return {
        createFunction(
            "جذر", "sqrt",
            "يحسب الجذر التربيعي للرقم",
            "دالة رياضية تحسب الجذر التربيعي للرقم المعطى. ترجع رقم عشري.",
            {createParam("الرقم", "رقم", "الرقم المراد حساب جذره التربيعي")},
            "رقم_عشري", "الجذر التربيعي للرقم", 75
        ),

        createFunction(
            "قوة", "power",
            "يرفع رقم إلى قوة معينة",
            "دالة رياضية ترفع الرقم الأول إلى قوة الرقم الثاني.",
            {
                createParam("الأساس", "رقم", "الرقم الأساس"),
                createParam("الأس", "رقم", "الأس أو القوة")
            },
            "رقم", "نتيجة رفع الأساس للأس", 75
        ),

        createFunction(
            "مطلق", "abs",
            "يحسب القيمة المطلقة للرقم",
            "دالة رياضية تحسب القيمة المطلقة (الموجبة) للرقم المعطى.",
            {createParam("الرقم", "رقم", "الرقم المراد حساب قيمته المطلقة")},
            "رقم", "القيمة المطلقة للرقم", 70
        ),

        createFunction(
            "عشوائي", "random",
            "يولد رقم عشوائي",
            "دالة تولد رقم عشوائي بين 0 و 1، أو بين حدين محددين.",
            {
                createParam("الحد_الأدنى", "رقم", "أصغر رقم ممكن", true, "0"),
                createParam("الحد_الأعلى", "رقم", "أكبر رقم ممكن", true, "1")
            },
            "رقم_عشري", "رقم عشوائي ضمن النطاق المحدد", 70
        )
    };
}

// String Completions
std::vector<ArabicCompletionItem> ArabicCompletionDatabase::getStringCompletions() {
    return {
        createFunction(
            "طول", "length",
            "يحسب طول النص",
            "دالة تحسب عدد الأحرف في النص المعطى.",
            {createParam("النص", "نص", "النص المراد حساب طوله")},
            "رقم", "عدد الأحرف في النص", 80
        ),

        createFunction(
            "يحتوي", "contains",
            "يتحقق من وجود نص فرعي داخل النص",
            "دالة تتحقق من وجود نص فرعي معين داخل النص الأساسي.",
            {
                createParam("النص_الأساسي", "نص", "النص المراد البحث فيه"),
                createParam("النص_الفرعي", "نص", "النص المراد البحث عنه")
            },
            "منطقي", "صحيح إذا وجد النص الفرعي، خطأ إذا لم يوجد", 75
        ),

        createFunction(
            "استبدل", "replace",
            "يستبدل نص بنص آخر",
            "دالة تستبدل جميع حالات النص القديم بالنص الجديد في النص الأساسي.",
            {
                createParam("النص_الأساسي", "نص", "النص المراد التعديل عليه"),
                createParam("النص_القديم", "نص", "النص المراد استبداله"),
                createParam("النص_الجديد", "نص", "النص البديل")
            },
            "نص", "النص بعد الاستبدال", 75
        )
    };
}

// Array Completions
std::vector<ArabicCompletionItem> ArabicCompletionDatabase::getArrayCompletions() {
    return {
        createKeyword(
            "مصفوفة", "array",
            "نوع بيانات لتخزين مجموعة من القيم",
            "نوع بيانات يستخدم لتخزين مجموعة مرتبة من القيم من نفس النوع أو أنواع مختلفة.",
            R"(// إنشاء مصفوفات مختلفة
مصفوفة الأسماء = ["أحمد", "فاطمة", "محمد"]
مصفوفة الأرقام = [1, 2, 3, 4, 5]
مصفوفة مختلطة = ["نص", 123, صحيح])",
            85
        ),

        createFunction(
            "أضف", "add",
            "يضيف عنصر جديد للمصفوفة",
            "دالة تضيف عنصر جديد في نهاية المصفوفة.",
            {
                createParam("المصفوفة", "مصفوفة", "المصفوفة المراد الإضافة إليها"),
                createParam("العنصر", "أي", "العنصر المراد إضافته")
            },
            "فراغ", "لا ترجع قيمة", 80
        ),

        createFunction(
            "احذف", "remove",
            "يحذف عنصر من المصفوفة",
            "دالة تحذف عنصر من المصفوفة بناءً على موقعه أو قيمته.",
            {
                createParam("المصفوفة", "مصفوفة", "المصفوفة المراد الحذف منها"),
                createParam("المؤشر", "رقم", "موقع العنصر المراد حذفه")
            },
            "فراغ", "لا ترجع قيمة", 75
        )
    };
}

// Function Completions
std::vector<ArabicCompletionItem> ArabicCompletionDatabase::getFunctionCompletions() {
    return {
        createKeyword(
            "دالة", "function",
            "يعرف دالة جديدة",
            "كلمة مفتاحية لتعريف دالة جديدة. الدالة هي مجموعة من الأوامر التي تنفذ مهمة محددة.",
            R"(// دالة بسيطة
دالة قل_مرحبا() {
    اطبع("مرحبا!")
}

// دالة مع معاملات
دالة اجمع(أ، ب) {
    ارجع أ + ب
})",
            90
        ),

        createKeyword(
            "ارجع", "return",
            "يرجع قيمة من الدالة",
            "كلمة مفتاحية ترجع قيمة من الدالة وتنهي تنفيذها.",
            R"(دالة اضرب(أ، ب) {
    متغير النتيجة = أ * ب
    ارجع النتيجة
})",
            85
        )
    };
}

// Class Completions
std::vector<ArabicCompletionItem> ArabicCompletionDatabase::getClassCompletions() {
    return {
        createKeyword(
            "فئة", "class",
            "يعرف فئة (كلاس) جديدة",
            "كلمة مفتاحية لتعريف فئة جديدة. الفئة هي قالب لإنشاء كائنات تحتوي على خصائص ودوال.",
            R"(فئة الشخص {
    // الخصائص
    خاص نص الاسم
    خاص رقم العمر

    // الباني
    دالة الشخص(اسم، عمر) {
        هذا.الاسم = اسم
        هذا.العمر = عمر
    }

    // دالة عامة
    عام دالة اعرض_المعلومات() {
        اطبع("الاسم:", هذا.الاسم, "العمر:", هذا.العمر)
    }
})",
            85
        ),

        createKeyword(
            "عام", "public",
            "يجعل العضو متاح للوصول من خارج الفئة",
            "كلمة مفتاحية تحدد أن الخاصية أو الدالة يمكن الوصول إليها من خارج الفئة.",
            R"(فئة المثال {
    عام نص الاسم  // يمكن الوصول إليه من الخارج
    عام دالة اعرض() {
        اطبع(هذا.الاسم)
    }
})",
            75
        ),

        createKeyword(
            "خاص", "private",
            "يجعل العضو متاح فقط داخل الفئة",
            "كلمة مفتاحية تحدد أن الخاصية أو الدالة يمكن الوصول إليها فقط من داخل الفئة نفسها.",
            R"(فئة المثال {
    خاص نص كلمة_المرور  // لا يمكن الوصول إليه من الخارج
    خاص دالة تحقق_من_الأمان() {
        // كود خاص بالفئة
    }
})",
            75
        )
    };
}

// Error Handling Completions
std::vector<ArabicCompletionItem> ArabicCompletionDatabase::getErrorHandlingCompletions() {
    return {
        createKeyword(
            "حاول", "try",
            "يحاول تنفيذ كود قد يسبب خطأ",
            "كلمة مفتاحية تبدأ كتلة من الكود الذي قد يسبب خطأ. يجب استخدامها مع 'اصطد'.",
            R"(حاول {
    متغير النتيجة = 10 / 0  // قد يسبب خطأ
    اطبع(النتيجة)
} اصطد (الخطأ) {
    اطبع("حدث خطأ:", الخطأ)
})",
            80
        ),

        createKeyword(
            "اصطد", "catch",
            "يصطاد الأخطاء التي تحدث في كتلة 'حاول'",
            "كلمة مفتاحية تصطاد الأخطاء التي تحدث في كتلة 'حاول' وتتعامل معها.",
            R"(حاول {
    // كود قد يسبب خطأ
} اصطد (الخطأ) {
    اطبع("تم اصطياد الخطأ:", الخطأ)
})",
            80
        )
    };
}

// File I/O Completions
std::vector<ArabicCompletionItem> ArabicCompletionDatabase::getFileIOCompletions() {
    return {
        createFunction(
            "اقرأ_ملف", "read_file",
            "يقرأ محتوى ملف",
            "دالة تقرأ محتوى ملف نصي وترجعه كنص.",
            {createParam("مسار_الملف", "نص", "مسار الملف المراد قراءته")},
            "نص", "محتوى الملف", 70
        ),

        createFunction(
            "اكتب_ملف", "write_file",
            "يكتب نص في ملف",
            "دالة تكتب النص المعطى في ملف. إذا كان الملف موجود، يتم استبدال محتواه.",
            {
                createParam("مسار_الملف", "نص", "مسار الملف المراد الكتابة فيه"),
                createParam("المحتوى", "نص", "النص المراد كتابته")
            },
            "منطقي", "صحيح إذا نجحت العملية، خطأ إذا فشلت", 70
        )
    };
}

// Control Flow Snippets
std::vector<CodeSnippet> ArabicCompletionDatabase::getControlFlowSnippets() {
    return {
        {
            "حلقة للعد",
            "حلقة for للعد من رقم إلى آخر",
            R"(لكل ${1:العداد} من ${2:1} إلى ${3:10} {
    ${4:// الكود هنا}
})",
            {"العداد", "1", "10", "// الكود هنا"},
            "control_flow",
            85,
            {"global", "function"}
        },

        {
            "حلقة عبر مصفوفة",
            "حلقة for للتكرار عبر عناصر مصفوفة",
            R"(لكل ${1:العنصر} في ${2:المصفوفة} {
    ${3:// معالجة العنصر}
})",
            {"العنصر", "المصفوفة", "// معالجة العنصر"},
            "control_flow",
            85,
            {"global", "function"}
        },

        {
            "شرط كامل",
            "جملة شرطية كاملة مع if-else if-else",
            R"(اذا (${1:الشرط_الأول}) {
    ${2:// الكود الأول}
} اواذا (${3:الشرط_الثاني}) {
    ${4:// الكود الثاني}
} والا {
    ${5:// الكود الافتراضي}
})",
            {"الشرط_الأول", "// الكود الأول", "الشرط_الثاني", "// الكود الثاني", "// الكود الافتراضي"},
            "control_flow",
            80,
            {"global", "function"}
        }
    };
}

// Function Snippets
std::vector<CodeSnippet> ArabicCompletionDatabase::getFunctionSnippets() {
    return {
        {
            "دالة جديدة",
            "إنشاء دالة جديدة مع معاملات",
            R"(دالة ${1:اسم_الدالة}(${2:المعاملات}) {
    ${3:// جسم الدالة}
    ارجع ${4:القيمة}
})",
            {"اسم_الدالة", "المعاملات", "// جسم الدالة", "القيمة"},
            "functions",
            80,
            {"global"}
        },

        {
            "دالة بدون إرجاع",
            "إنشاء دالة لا ترجع قيمة",
            R"(دالة ${1:اسم_الدالة}(${2:المعاملات}) {
    ${3:// جسم الدالة}
})",
            {"اسم_الدالة", "المعاملات", "// جسم الدالة"},
            "functions",
            75,
            {"global"}
        }
    };
}

// Class Snippets
std::vector<CodeSnippet> ArabicCompletionDatabase::getClassSnippets() {
    return {
        {
            "فئة جديدة",
            "إنشاء فئة (class) جديدة",
            R"(فئة ${1:اسم_الفئة} {
    // الخصائص
    ${2:خاص متغير القيمة}

    // الباني
    دالة ${1:اسم_الفئة}(${3:المعاملات}) {
        ${4:// كود الباني}
    }

    // الدوال
    ${5:// دوال الفئة}
})",
            {"اسم_الفئة", "خاص متغير القيمة", "المعاملات", "// كود الباني", "// دوال الفئة"},
            "classes",
            75,
            {"global"}
        }
    };
}

// Common Pattern Snippets
std::vector<CodeSnippet> ArabicCompletionDatabase::getCommonPatternSnippets() {
    return {
        {
            "برنامج رئيسي",
            "هيكل البرنامج الرئيسي",
            R"(// ${1:اسم البرنامج}
// ${2:وصف البرنامج}

دالة رئيسية() {
    ${3:// كود البرنامج الرئيسي}
}

// تشغيل البرنامج
رئيسية())",
            {"اسم البرنامج", "وصف البرنامج", "// كود البرنامج الرئيسي"},
            "common",
            90,
            {"global"}
        },

        {
            "معالجة الأخطاء",
            "نمط معالجة الأخطاء الأساسي",
            R"(حاول {
    ${1:// الكود الذي قد يسبب خطأ}
} اصطد (${2:الخطأ}) {
    اطبع("حدث خطأ:", ${2:الخطأ})
    ${3:// معالجة الخطأ}
})",
            {"// الكود الذي قد يسبب خطأ", "الخطأ", "// معالجة الخطأ"},
            "error_handling",
            75,
            {"global", "function"}
        }
    };
}

} // namespace features
} // namespace als
