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

struct Modifier {
    const char *xkbModifier;
    Qt::KeyboardModifier qtModifier;
};

std::optional<QKeySequence> XdgShortcut::parse(const QString &shortcutString)
{
    static const QHash<QString, Modifier> allowedModifiers = {
        {u"SHIFT"_s, {XKB_MOD_NAME_SHIFT, Qt::ShiftModifier}},
        {u"CAPS"_s, {XKB_MOD_NAME_CAPS, Qt::GroupSwitchModifier}},
        {u"CTRL"_s, {XKB_MOD_NAME_CTRL, Qt::ControlModifier}},
        {u"ALT"_s, {XKB_MOD_NAME_ALT, Qt::AltModifier}},
        {u"NUM"_s, {XKB_MOD_NAME_NUM, Qt::KeypadModifier}},
        {u"LOGO"_s, {XKB_MOD_NAME_LOGO, Qt::MetaModifier}},
    };

    xkb_keysym_t identifier = XKB_KEY_NoSymbol;
    QStringList modifiers;

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
            if (!allowedModifiers.contains(modifier.toString())) {
                qWarning() << "Unknown modifier" << modifier;
                return {};
            }

            modifiers << modifier.toString();
            remaining = remaining.mid(nextPlus + 1);
        }
    }

    int keys = 0;
    for (const QString &modifier : std::as_const(modifiers)) {
        keys |= allowedModifiers[modifier].qtModifier;
    }

    keys |= QXkbCommon::keysymToQtKey(identifier, Qt::NoModifier, nullptr, XKB_KEYCODE_INVALID);
    return QKeySequence(keys);
}
