/*
 * SPDX-FileCopyrightText: 2022 Aleix Pol i Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "xdgshortcut.h"
#include <QDebug>
#include <QRegularExpression>
#include <QtGui/private/qxkbcommon_p.h>

using namespace Qt::StringLiterals;

std::optional<QKeySequence> XdgShortcut::parse(const QString &shortcutString)
{
    static const QHash<QString, Qt::KeyboardModifier> allowedModifiers = {
        {u"SHIFT"_s, Qt::ShiftModifier},
        {u"CAPS"_s, Qt::GroupSwitchModifier},
        {u"CTRL"_s, Qt::ControlModifier},
        {u"ALT"_s, Qt::AltModifier},
        {u"NUM"_s, Qt::KeypadModifier},
        {u"LOGO"_s, Qt::MetaModifier},
    };

    xkb_keysym_t identifier = XKB_KEY_NoSymbol;
    Qt::KeyboardModifiers modifiers;

    QStringView remaining(shortcutString);
    while (!remaining.isEmpty()) {
        auto nextPlus = remaining.indexOf(u'+');
        if (nextPlus == 0) { // ++ or ending with +
            qWarning() << "empty modifier";
            return {};
        }

        if (nextPlus < 0) { // just the identifier left
            // The spec says that the string ends when all the spec'ed characters are over
            // Meaning "CTRL+a;Banana" would be an acceptable and parseable string
            static QRegularExpression rx(QStringLiteral("^([\\w\\d_]+).*$"));
            Q_ASSERT(rx.isValid());
            rx.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
            QRegularExpressionMatch match = rx.match(remaining);
            Q_ASSERT(match.isValid());

            identifier = xkb_keysym_from_name(match.capturedView(1).toUtf8().constData(), XKB_KEYSYM_CASE_INSENSITIVE);
            if (identifier == XKB_KEY_NoSymbol) {
                qWarning() << "unknown key" << match.capturedView(1);
                return false;
            }
            break;
        } else { // A modifier
            auto modifier = remaining.left(nextPlus);
            auto it = allowedModifiers.find(modifier.toString());
            if (it == allowedModifiers.cend()) {
                qWarning() << "Unknown modifier" << modifier;
                return {};
            }

            modifiers |= it.value();
            remaining = remaining.mid(nextPlus + 1);
        }
    }

    int keys = QXkbCommon::keysymToQtKey(identifier, Qt::NoModifier, nullptr, XKB_KEYCODE_INVALID);
    return QKeySequence(modifiers | keys);
}
