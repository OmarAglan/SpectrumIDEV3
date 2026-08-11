#include "LspMessageFramer.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <array>

#if defined(Q_OS_WIN)
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace {
void send(QFile &output, const QJsonObject &message)
{
    const QByteArray body = QJsonDocument(message).toJson(QJsonDocument::Compact);
    output.write(LspMessageFramer::frame(body));
    output.flush();
}

void publish(QFile &output, const QString &uri, int version)
{
    const QJsonObject diagnostic{
        {"range", QJsonObject{
            {"start", QJsonObject{{"line", 1}, {"character", 4}}},
            {"end", QJsonObject{{"line", 1}, {"character", 9}}}
        }},
        {"severity", 1},
        {"code", "B1000"},
        {"source", "باء"},
        {"message", "رمز غير معرّف"},
        {"data", QJsonObject{{"category", "semantic"}, {"hint", "عرّف الرمز"}}}
    };
    send(output, QJsonObject{
        {"jsonrpc", "2.0"},
        {"method", "textDocument/publishDiagnostics"},
        {"params", QJsonObject{
            {"uri", uri}, {"version", version},
            {"diagnostics", QJsonArray{diagnostic}}
        }}
    });
}

bool shouldCrashAfterOpen()
{
    if (qEnvironmentVariableIntValue("QALAM_FAKE_LSP_ALWAYS_CRASH") != 0) {
        return true;
    }

    const QString markerPath =
        QString::fromUtf8(qgetenv("QALAM_FAKE_LSP_CRASH_ONCE_MARKER"));
    if (markerPath.isEmpty() or QFile::exists(markerPath)) return false;
    QFile marker(markerPath);
    if (not marker.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    marker.write("crashed\n");
    marker.close();
    return true;
}

void recordWorkspaceMessage(const QJsonObject &message)
{
    const QString path =
        QString::fromUtf8(qgetenv("QALAM_FAKE_LSP_WORKSPACE_LOG"));
    if (path.isEmpty()) return;
    QFile log(path);
    if (not log.open(QIODevice::WriteOnly | QIODevice::Append)) return;
    log.write(QJsonDocument(message).toJson(QJsonDocument::Compact));
    log.write("\n");
}
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    QFile output;
    if (not output.open(stdout, QIODevice::WriteOnly)) return 4;
#if defined(Q_OS_WIN)
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    LspMessageFramer framer;
    std::array<char, 4096> buffer{};
    QString currentUri;
    while (true) {
#if defined(Q_OS_WIN)
        const int count = _read(_fileno(stdin), buffer.data(),
                                static_cast<unsigned>(buffer.size()));
#else
        const ssize_t count = ::read(STDIN_FILENO, buffer.data(), buffer.size());
#endif
        if (count <= 0) return 0;
        const QByteArray chunk(buffer.data(), static_cast<qsizetype>(count));
        QString frameError;
        const QList<QByteArray> messages = framer.appendData(chunk, &frameError);
        if (not frameError.isEmpty()) return 2;

        for (const QByteArray &body : messages) {
            const QJsonObject message = QJsonDocument::fromJson(body).object();
            const QString method = message.value("method").toString();
            const QJsonValue id = message.value("id");
            const QJsonObject params = message.value("params").toObject();

            if (method == "initialize") {
                recordWorkspaceMessage(message);
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"}, {"id", id},
                    {"result", QJsonObject{
                        {"capabilities", QJsonObject{
                            {"positionEncoding", "utf-16"},
                            {"textDocumentSync", QJsonObject{{"openClose", true}, {"change", 1}}},
                            {"documentSymbolProvider", true},
                            {"foldingRangeProvider", true},
                            {"selectionRangeProvider", true},
                            {"semanticTokensProvider", QJsonObject{
                                {"legend", QJsonObject{
                                    {"tokenTypes", QJsonArray{
                                        "type", "macro", "keyword",
                                        "modifier", "comment", "string",
                                        "number", "operator", "function",
                                        "variable", "parameter", "property",
                                        "enumMember"
                                    }},
                                    {"tokenModifiers", QJsonArray{}}
                                }},
                                {"full", true}
                            }},
                            {"workspaceSymbolProvider", true},
                            {"workspace", QJsonObject{
                                {"workspaceFolders", QJsonObject{
                                    {"supported", true},
                                    {"changeNotifications", true}
                                }}
                            }},
                            {"hoverProvider", true},
                            {"definitionProvider", true},
                            {"referencesProvider", true},
                            {"codeActionProvider", QJsonObject{
                                {"codeActionKinds", QJsonArray{"quickfix"}},
                                {"resolveProvider", false}
                            }},
                            {"documentFormattingProvider", true},
                            {"renameProvider", QJsonObject{
                                {"prepareProvider", true}
                            }},
                            {"signatureHelpProvider", QJsonObject{
                                {"triggerCharacters", QJsonArray{"(", "،", ","}}
                            }},
                            {"completionProvider", QJsonObject{
                                {"resolveProvider", false},
                                {"triggerCharacters", QJsonArray{"ا", "#"}}
                            }}
                        }},
                        {"serverInfo", QJsonObject{{"name", "Fake Baa-LSP"}, {"version", "test"}}}
                    }}
                });
            } else if (method == "textDocument/didOpen") {
                const QJsonObject document = params.value("textDocument").toObject();
                currentUri = document.value("uri").toString();
                if (shouldCrashAfterOpen()) return 86;
                publish(output, document.value("uri").toString(), document.value("version").toInt());
            } else if (method == "textDocument/didChange") {
                const QJsonObject document = params.value("textDocument").toObject();
                currentUri = document.value("uri").toString();
                const int version = document.value("version").toInt();
                publish(output, document.value("uri").toString(), version - 1);
                publish(output, document.value("uri").toString(), version);
            } else if (method == "workspace/didChangeWorkspaceFolders" or
                       method == "workspace/didChangeWatchedFiles") {
                recordWorkspaceMessage(message);
            } else if (method == "textDocument/documentSymbol") {
                const QJsonObject range{
                    {"start", QJsonObject{{"line", 0}, {"character", 5}}},
                    {"end", QJsonObject{{"line", 0}, {"character", 13}}}
                };
                const QJsonObject symbol{
                    {"name", "الرئيسية"},
                    {"detail", "-> صحيح"},
                    {"kind", 12},
                    {"range", range},
                    {"selectionRange", range}
                };
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"}, {"id", id},
                    {"result", QJsonArray{symbol}}
                });
            } else if (method ==
                       "textDocument/semanticTokens/full") {
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"},
                    {"id", id},
                    {"result", QJsonObject{
                        {"data", QJsonArray{
                            0, 0, 4, 0, 0,
                            0, 5, 8, 8, 0,
                            1, 12, 1, 6, 0
                        }}
                    }}
                });
            } else if (method == "textDocument/foldingRange") {
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"},
                    {"id", id},
                    {"result", QJsonArray{QJsonObject{
                        {"startLine", 0},
                        {"startCharacter", 15},
                        {"endLine", 2},
                        {"endCharacter", 1},
                        {"kind", "region"}
                    }}}
                });
            } else if (method == "textDocument/selectionRange") {
                const QJsonObject documentRange{
                    {"start", QJsonObject{{"line", 0}, {"character", 0}}},
                    {"end", QJsonObject{{"line", 3}, {"character", 0}}}
                };
                const QJsonObject groupRange{
                    {"start", QJsonObject{{"line", 0}, {"character", 15}}},
                    {"end", QJsonObject{{"line", 2}, {"character", 1}}}
                };
                const QJsonObject lineRange{
                    {"start", QJsonObject{{"line", 1}, {"character", 4}}},
                    {"end", QJsonObject{{"line", 1}, {"character", 13}}}
                };
                const QJsonObject tokenRange{
                    {"start", QJsonObject{{"line", 1}, {"character", 4}}},
                    {"end", QJsonObject{{"line", 1}, {"character", 9}}}
                };
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"},
                    {"id", id},
                    {"result", QJsonArray{QJsonObject{
                        {"range", tokenRange},
                        {"parent", QJsonObject{
                            {"range", lineRange},
                            {"parent", QJsonObject{
                                {"range", groupRange},
                                {"parent", QJsonObject{
                                    {"range", documentRange}
                                }}
                            }}
                        }}
                    }}}
                });
            } else if (method == "workspace/symbol") {
                if (params.value("query").toString() ==
                    QStringLiteral("خطأ")) {
                    send(output, QJsonObject{
                        {"jsonrpc", "2.0"},
                        {"id", id},
                        {"error", QJsonObject{
                            {"code", -32801},
                            {"message", "Workspace index changed"}
                        }}
                    });
                    continue;
                }
                const QJsonObject mainRange{
                    {"start", QJsonObject{{"line", 0}, {"character", 5}}},
                    {"end", QJsonObject{{"line", 0}, {"character", 13}}}
                };
                const QJsonObject fieldRange{
                    {"start", QJsonObject{{"line", 1}, {"character", 4}}},
                    {"end", QJsonObject{{"line", 1}, {"character", 14}}}
                };
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"}, {"id", id},
                    {"result", QJsonArray{
                        QJsonObject{
                            {"name", "الرئيسية"},
                            {"kind", 12},
                            {"location", QJsonObject{
                                {"uri", currentUri}, {"range", mainRange}
                            }},
                            {"data", QJsonObject{
                                {"baaKind", "function"},
                                {"detail", "صحيح الرئيسية()"}
                            }}
                        },
                        QJsonObject{
                            {"name", "قيمة_عضو"},
                            {"kind", 8},
                            {"containerName", "سجل"},
                            {"location", QJsonObject{
                                {"uri", currentUri}, {"range", fieldRange}
                            }},
                            {"data", QJsonObject{
                                {"baaKind", "field"},
                                {"detail", "صحيح"}
                            }}
                        }
                    }}
                });
            } else if (method == "textDocument/completion") {
                const QJsonObject position = params.value("position").toObject();
                const int line = position.value("line").toInt();
                const int character = position.value("character").toInt();
                const QJsonObject range{
                    {"start", QJsonObject{{"line", line}, {"character", 5}}},
                    {"end", QJsonObject{{"line", line}, {"character", character}}}
                };
                const QJsonObject item{
                    {"label", "الرئيسية"},
                    {"detail", "دالة ← صحيح"},
                    {"filterText", "الرئيسية"},
                    {"sortText", "2الرئيسية"},
                    {"kind", 3},
                    {"insertTextFormat", 1},
                    {"textEdit", QJsonObject{
                        {"range", range}, {"newText", "الرئيسية"}
                    }}
                };
                const QJsonObject localItem{
                    {"label", "قيمة_محلية"},
                    {"detail", "صحيح قيمة_محلية"},
                    {"filterText", "قيمة_محلية"},
                    {"sortText", "0قيمة_محلية"},
                    {"kind", 6},
                    {"insertTextFormat", 1},
                    {"textEdit", QJsonObject{
                        {"range", range}, {"newText", "قيمة_محلية"}
                    }}
                };
                const QJsonObject includedItem{
                    {"label", "من_واجهة"},
                    {"detail", "صحيح من_واجهة()"},
                    {"filterText", "من_واجهة"},
                    {"sortText", "1من_واجهة"},
                    {"kind", 3},
                    {"insertTextFormat", 1},
                    {"textEdit", QJsonObject{
                        {"range", range}, {"newText", "من_واجهة"}
                    }}
                };
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"}, {"id", id},
                    {"result", QJsonObject{
                        {"isIncomplete", false},
                        {"items", QJsonArray{
                            item, localItem, includedItem
                        }}
                    }}
                });
            } else if (method == "textDocument/hover") {
                const QJsonObject position = params.value("position").toObject();
                const int line = position.value("line").toInt();
                const int character = position.value("character").toInt();
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"}, {"id", id},
                    {"result", QJsonObject{
                        {"contents", QJsonObject{
                            {"kind", "markdown"},
                            {"value", "```baa\nصحيح اجمع(صحيح أول، صحيح ثان)\n```\n\nدالة باء"}
                        }},
                        {"range", QJsonObject{
                            {"start", QJsonObject{{"line", line},
                                                 {"character", character}}},
                            {"end", QJsonObject{{"line", line},
                                               {"character", character + 4}}}
                        }}
                    }}
                });
            } else if (method == "textDocument/signatureHelp") {
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"}, {"id", id},
                    {"result", QJsonObject{
                        {"signatures", QJsonArray{
                            QJsonObject{
                                {"label", "صحيح اجمع(صحيح أول، صحيح ثان)"},
                                {"parameters", QJsonArray{
                                    QJsonObject{{"label", "صحيح أول"}},
                                    QJsonObject{{"label", "صحيح ثان"}}
                                }}
                            }
                        }},
                        {"activeSignature", 0},
                        {"activeParameter", 1}
                    }}
                });
            } else if (method == "textDocument/definition") {
                const QString uri = params.value("textDocument").toObject()
                                        .value("uri").toString();
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"}, {"id", id},
                    {"result", QJsonObject{
                        {"uri", uri},
                        {"range", QJsonObject{
                            {"start", QJsonObject{{"line", 0}, {"character", 5}}},
                            {"end", QJsonObject{{"line", 0}, {"character", 9}}}
                        }}
                    }}
                });
            } else if (method == "textDocument/references") {
                const QString uri = params.value("textDocument").toObject()
                                        .value("uri").toString();
                const QJsonObject declaration{
                    {"uri", uri},
                    {"range", QJsonObject{
                        {"start", QJsonObject{{"line", 0}, {"character", 5}}},
                        {"end", QJsonObject{{"line", 0}, {"character", 9}}}
                    }}
                };
                const QJsonObject use{
                    {"uri", uri},
                    {"range", QJsonObject{
                        {"start", QJsonObject{{"line", 1}, {"character", 8}}},
                        {"end", QJsonObject{{"line", 1}, {"character", 12}}}
                    }}
                };
                QJsonArray results{use};
                if (params.value("context").toObject()
                        .value("includeDeclaration").toBool()) {
                    results.prepend(declaration);
                }
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"}, {"id", id}, {"result", results}
                });
            } else if (method == "textDocument/formatting") {
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"}, {"id", id},
                    {"result", QJsonArray{
                        QJsonObject{
                            {"range", QJsonObject{
                                {"start", QJsonObject{
                                    {"line", 0}, {"character", 0}
                                }},
                                {"end", QJsonObject{
                                    {"line", 3}, {"character", 0}
                                }}
                            }},
                            {"newText",
                             "صحيح الرئيسية() {\n    أرجع ٠.\n}\n"}
                        }
                    }}
                });
            } else if (method == "textDocument/codeAction") {
                const QString uri = params.value("textDocument").toObject()
                                        .value("uri").toString();
                const QJsonObject insertion{
                    {"start", QJsonObject{{"line", 1}, {"character", 4}}},
                    {"end", QJsonObject{{"line", 1}, {"character", 4}}}
                };
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"}, {"id", id},
                    {"result", QJsonArray{
                        QJsonObject{
                            {"title", "عرّف المتغير بإضافة نوعه"},
                            {"kind", "quickfix"},
                            {"isPreferred", true},
                            {"data", QJsonObject{
                                {"fixId", "B1000.insert-int-type"}
                            }},
                            {"edit", QJsonObject{
                                {"documentChanges", QJsonArray{
                                    QJsonObject{
                                        {"textDocument", QJsonObject{
                                            {"uri", uri}, {"version", 1}
                                        }},
                                        {"edits", QJsonArray{
                                            QJsonObject{
                                                {"range", insertion},
                                                {"newText", "صحيح "}
                                            }
                                        }}
                                    }
                                }}
                            }}
                        }
                    }}
                });
            } else if (method == "textDocument/prepareRename") {
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"}, {"id", id},
                    {"result", QJsonObject{
                        {"range", QJsonObject{
                            {"start", QJsonObject{{"line", 1}, {"character", 8}}},
                            {"end", QJsonObject{{"line", 1}, {"character", 12}}}
                        }},
                        {"placeholder", "اجمع"}
                    }}
                });
            } else if (method == "textDocument/rename") {
                const QString uri = params.value("textDocument").toObject()
                                        .value("uri").toString();
                const QString newName = params.value("newName").toString();
                send(output, QJsonObject{
                    {"jsonrpc", "2.0"}, {"id", id},
                    {"result", QJsonObject{
                        {"documentChanges", QJsonArray{
                            QJsonObject{
                                {"textDocument", QJsonObject{
                                    {"uri", uri}, {"version", 1}
                                }},
                                {"edits", QJsonArray{
                                    QJsonObject{
                                        {"range", QJsonObject{
                                            {"start", QJsonObject{{"line", 0}, {"character", 5}}},
                                            {"end", QJsonObject{{"line", 0}, {"character", 9}}}
                                        }},
                                        {"newText", newName}
                                    },
                                    QJsonObject{
                                        {"range", QJsonObject{
                                            {"start", QJsonObject{{"line", 1}, {"character", 8}}},
                                            {"end", QJsonObject{{"line", 1}, {"character", 12}}}
                                        }},
                                        {"newText", newName}
                                    }
                                }}
                            }
                        }}
                    }}
                });
            } else if (method == "shutdown") {
                send(output, QJsonObject{{"jsonrpc", "2.0"}, {"id", id}, {"result", QJsonValue::Null}});
            } else if (method == "exit") {
                return 0;
            }
        }
    }
}
