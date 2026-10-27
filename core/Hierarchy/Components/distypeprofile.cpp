// =============================================================================
// FILE:        distypeprofile.cpp
// MODULE:      Tactical Simulation Entity Management
// PROJECT:     Tactical Display/Simulation Framework (TDF)
// ORGANISATION: Oxygen 2 Innovation (O2I)
//
// DESCRIPTION: Implementation of DISTypeProfile — see distypeprofile.h.
//
// COPYRIGHT:    Oxygen 2 Innovation (O2I). All rights reserved.
// =============================================================================

#include "distypeprofile.h"
#include "core/DISPlugin/config/disreferencedataloader.h"
#include <core/Utility/uuid.h>

/**
 * @brief Constructs a DISTypeProfile with safe, inert defaults.
 *        useOverride starts false — this component has no effect on the
 *        entity's outgoing DIS PDU until explicitly enabled.
 */
DISTypeProfile::DISTypeProfile() : Component()
{
    ID = Uuid::generateShortUniqueID();
}

/**
 * @brief Binds this component to its owning entity. No ECS components
 *        are required for this profile — it is pure configuration data
 *        consumed directly by the DIS plugin.
 * @param entity Owning entity.
 */
void DISTypeProfile::init(Entity* entity)
{
    parentEntity = entity;
}

void DISTypeProfile::Awake() {}
void DISTypeProfile::Start() {}

// =============================================================================
// Sub-component management — not applicable to this component (matches
// Transform's pattern of no-op overrides for leaf components).
// =============================================================================
void DISTypeProfile::addSubComponent(std::string name, QString data1, QString data2, QJsonObject data3) {}
void DISTypeProfile::removeSubComponent(std::string ID) {}
QJsonObject DISTypeProfile::getsubComponentData(std::string ID) const { return QJsonObject(); }
void DISTypeProfile::updateSubComponent(std::string ID, const QJsonObject& obj) {}

// =============================================================================
// formatEntry
// Generic "value -> display string" lookup shared by every field. Falls
// back to the bare numeric value (still zero-padded per padWidth) if the
// table has no matching entry — this is what makes an unpopulated
// SubCategory/Specific/Extra branch stay usable rather than broken.
// =============================================================================
QString DISTypeProfile::formatEntry(const std::vector<DISRefEntry>& table,
                                    uint16_t value, int padWidth)
{
    for (const DISRefEntry& e : table) {
        if (e.value == value) {
            return QString("%1 - %2")
            .arg(value, padWidth, 10, QChar('0'))
                .arg(QString::fromStdString(e.name));
        }
    }
    return QString("%1").arg(value, padWidth, 10, QChar('0'));
}

/**
 * @brief Extracts the leading integer from a "NNN - Name" display string,
 *        or a plain numeric string. Used to parse dropdown/edited values
 *        back into raw SISO codes.
 */
uint16_t DISTypeProfile::parseLeadingNumber(const QString& displayValue)
{
    QString digits;
    for (QChar c : displayValue) {
        if (c.isDigit()) {
            digits += c;
        } else if (!digits.isEmpty()) {
            break;
        }
    }
    bool ok = false;
    uint16_t value = static_cast<uint16_t>(digits.toUInt(&ok));
    return ok ? value : 0;
}

// =============================================================================
// SECTION: Kind — global, flat, always fully populated.
// =============================================================================
QJsonArray DISTypeProfile::kindOptions() const
{
    const DISReferenceData& ref = DISReferenceDataLoader::instance();
    QJsonArray arr;
    for (const DISRefEntry& e : ref.kind)
        arr.append(formatEntry(ref.kind, e.value, 3));
    return arr;
}

QString DISTypeProfile::kindName(uint8_t value) const
{
    return formatEntry(DISReferenceDataLoader::instance().kind, value, 3);
}

// =============================================================================
// SECTION: Domain — depends on the currently-set Kind.
// =============================================================================
QJsonArray DISTypeProfile::domainOptions() const
{
    const DISReferenceData& ref = DISReferenceDataLoader::instance();
    QJsonArray arr;
    auto it = ref.domain.find(kind);
    if (it != ref.domain.end())
        for (const DISRefEntry& e : it->second)
            arr.append(formatEntry(it->second, e.value, 3));
    return arr;
}

QString DISTypeProfile::domainName(uint8_t value) const
{
    const DISReferenceData& ref = DISReferenceDataLoader::instance();
    auto it = ref.domain.find(kind);
    if (it != ref.domain.end())
        return formatEntry(it->second, value, 3);
    return QString("%1").arg(value, 3, 10, QChar('0'));
}

// =============================================================================
// SECTION: Country — global, flat, independent of Kind/Domain.
// =============================================================================
QJsonArray DISTypeProfile::countryOptions() const
{
    const DISReferenceData& ref = DISReferenceDataLoader::instance();
    QJsonArray arr;
    for (const DISRefEntry& e : ref.country)
        arr.append(formatEntry(ref.country, e.value, 4));
    return arr;
}

QString DISTypeProfile::countryName(uint16_t value) const
{
    return formatEntry(DISReferenceDataLoader::instance().country, value, 4);
}

// =============================================================================
// SECTION: Category — depends on the currently-set Kind + Domain.
// Confirmed against the real SISO-REF-010 v35 data that Category codes
// are consistent regardless of Country, so Country is not part of this
// lookup key.
// =============================================================================
QJsonArray DISTypeProfile::categoryOptions() const
{
    const DISReferenceData& ref = DISReferenceDataLoader::instance();
    QJsonArray arr;
    auto kIt = ref.category.find(kind);
    if (kIt != ref.category.end()) {
        auto dIt = kIt->second.find(domain);
        if (dIt != kIt->second.end())
            for (const DISRefEntry& e : dIt->second)
                arr.append(formatEntry(dIt->second, e.value, 0));
    }
    return arr;
}

QString DISTypeProfile::categoryName(uint8_t value) const
{
    const DISReferenceData& ref = DISReferenceDataLoader::instance();
    auto kIt = ref.category.find(kind);
    if (kIt != ref.category.end()) {
        auto dIt = kIt->second.find(domain);
        if (dIt != kIt->second.end())
            return formatEntry(dIt->second, value, 0);
    }
    return QString::number(value);
}

// =============================================================================
// SECTION: SubCategory — depends on Kind + Domain + Category. Empty until
// entries are added to siso_reference_data.json by hand (self-service).
// =============================================================================
QJsonArray DISTypeProfile::subcategoryOptions() const
{
    const DISReferenceData& ref = DISReferenceDataLoader::instance();
    QJsonArray arr;
    auto kIt = ref.subcategory.find(kind);
    if (kIt != ref.subcategory.end()) {
        auto dIt = kIt->second.find(domain);
        if (dIt != kIt->second.end()) {
            auto cIt = dIt->second.find(category);
            if (cIt != dIt->second.end())
                for (const DISRefEntry& e : cIt->second)
                    arr.append(formatEntry(cIt->second, e.value, 0));
        }
    }
    return arr;
}

QString DISTypeProfile::subcategoryName(uint8_t value) const
{
    const DISReferenceData& ref = DISReferenceDataLoader::instance();
    auto kIt = ref.subcategory.find(kind);
    if (kIt != ref.subcategory.end()) {
        auto dIt = kIt->second.find(domain);
        if (dIt != kIt->second.end()) {
            auto cIt = dIt->second.find(category);
            if (cIt != dIt->second.end())
                return formatEntry(cIt->second, value, 0);
        }
    }
    return QString::number(value);
}

// =============================================================================
// SECTION: Specific — depends on Kind + Domain + Category + SubCategory.
// Empty until entries are added to siso_reference_data.json by hand.
// =============================================================================
QJsonArray DISTypeProfile::specificOptions() const
{
    const DISReferenceData& ref = DISReferenceDataLoader::instance();
    QJsonArray arr;
    auto kIt = ref.specific.find(kind);
    if (kIt != ref.specific.end()) {
        auto dIt = kIt->second.find(domain);
        if (dIt != kIt->second.end()) {
            auto cIt = dIt->second.find(category);
            if (cIt != dIt->second.end()) {
                auto sIt = cIt->second.find(subcategory);
                if (sIt != cIt->second.end())
                    for (const DISRefEntry& e : sIt->second)
                        arr.append(formatEntry(sIt->second, e.value, 0));
            }
        }
    }
    return arr;
}

QString DISTypeProfile::specificName(uint8_t value) const
{
    const DISReferenceData& ref = DISReferenceDataLoader::instance();
    auto kIt = ref.specific.find(kind);
    if (kIt != ref.specific.end()) {
        auto dIt = kIt->second.find(domain);
        if (dIt != kIt->second.end()) {
            auto cIt = dIt->second.find(category);
            if (cIt != dIt->second.end()) {
                auto sIt = cIt->second.find(subcategory);
                if (sIt != cIt->second.end())
                    return formatEntry(sIt->second, value, 0);
            }
        }
    }
    return QString::number(value);
}

// =============================================================================
// SECTION: Extra — same lookup depth as Specific (Kind + Domain + Category
// + SubCategory). Extremely rarely populated in the real standard; extend
// one level deeper (keyed by Specific too) only if a real need arises.
// =============================================================================
QJsonArray DISTypeProfile::extraOptions() const
{
    const DISReferenceData& ref = DISReferenceDataLoader::instance();
    QJsonArray arr;
    auto kIt = ref.extra.find(kind);
    if (kIt != ref.extra.end()) {
        auto dIt = kIt->second.find(domain);
        if (dIt != kIt->second.end()) {
            auto cIt = dIt->second.find(category);
            if (cIt != dIt->second.end()) {
                auto sIt = cIt->second.find(subcategory);
                if (sIt != cIt->second.end())
                    for (const DISRefEntry& e : sIt->second)
                        arr.append(formatEntry(sIt->second, e.value, 0));
            }
        }
    }
    if (arr.isEmpty()) {
        // Matches STAGE's own Extra field, which offers exactly these two
        // values today even with no deeper data populated.
        arr.append("0");
        arr.append("255");
    }
    return arr;
}

QString DISTypeProfile::extraName(uint8_t value) const
{
    const DISReferenceData& ref = DISReferenceDataLoader::instance();
    auto kIt = ref.extra.find(kind);
    if (kIt != ref.extra.end()) {
        auto dIt = kIt->second.find(domain);
        if (dIt != kIt->second.end()) {
            auto cIt = dIt->second.find(category);
            if (cIt != dIt->second.end()) {
                auto sIt = cIt->second.find(subcategory);
                if (sIt != cIt->second.end())
                    return formatEntry(sIt->second, value, 0);
            }
        }
    }
    return QString::number(value);
}

/**
 * @brief Serializes this profile to the property-grid JSON shape already
 *        used elsewhere in TDF (Platform::toJson()'s Category/SubCategory
 *        fields follow this same {"type":"option",...} pattern).
 *        "refresh":true on Kind/Domain/Category/SubCategory means changing
 *        any of them re-renders the panel with the next field's options
 *        correctly filtered — same dependent-dropdown mechanism already
 *        proven working for Platform's Category -> SubCategory today.
 */

QJsonObject DISTypeProfile::toJson() const
{
    QJsonObject obj;
    obj["id"]   = QString::fromStdString(ID);
    obj["type"] = "component";

    obj["useOverride"] = useOverride;

    QJsonObject kindObj;
    kindObj["type"]     = "option";
    kindObj["editable"] = true;
    kindObj["refresh"]  = true;
    kindObj["options"]  = kindOptions();
    kindObj["value"]    = kindName(kind);
    obj["Kind"] = kindObj;

    QJsonObject domainObj;
    domainObj["type"]     = "option";
    domainObj["editable"] = true;
    domainObj["refresh"]  = true;
    domainObj["options"]  = domainOptions();
    domainObj["value"]    = domainName(domain);
    obj["Domain"] = domainObj;

    QJsonObject countryObj;
    countryObj["type"]     = "option";
    countryObj["editable"] = true;
    countryObj["options"]  = countryOptions();
    countryObj["value"]    = countryName(country);
    obj["Country"] = countryObj;

    QJsonObject categoryObj;
    categoryObj["type"]     = "option";
    categoryObj["editable"] = true;
    categoryObj["refresh"]  = true;
    categoryObj["options"]  = categoryOptions();
    categoryObj["value"]    = categoryName(category);
    obj["Category"] = categoryObj;

    QJsonObject subCategoryObj;
    subCategoryObj["type"]     = "option";
    subCategoryObj["editable"] = true;
    subCategoryObj["refresh"]  = true;
    subCategoryObj["options"]  = subcategoryOptions();
    subCategoryObj["value"]    = subcategoryName(subcategory);
    obj["SubCategory"] = subCategoryObj;

    QJsonObject specificObj;
    specificObj["type"]     = "option";
    specificObj["editable"] = true;
    specificObj["options"]  = specificOptions();
    specificObj["value"]    = specificName(specific);
    obj["Specific"] = specificObj;

    QJsonObject extraObj;
    extraObj["type"]     = "option";
    extraObj["editable"] = true;
    extraObj["options"]  = extraOptions();
    extraObj["value"]    = extraName(extra);
    obj["Extra"] = extraObj;
    QString disTypeReadout = QString("%1-%2-%3-%4-%5-%6-%7")
                                 .arg(kind, 3, 10, QChar('0'))
                                 .arg(domain, 3, 10, QChar('0'))
                                 .arg(country, 4, 10, QChar('0'))
                                 .arg(category, 3, 10, QChar('0'))
                                 .arg(subcategory, 3, 10, QChar('0'))
                                 .arg(specific, 3, 10, QChar('0'))
                                 .arg(extra, 3, 10, QChar('0'));
    obj["DIS Type"] = disTypeReadout;
    return obj;

}

/**
 * @brief Deserializes this profile from the property-grid JSON shape.
 *        Accepts either the full {"type":"option",...} wrapper (as saved
 *        by toJson()) or a bare value, for robustness.
 */
void DISTypeProfile::fromJson(const QJsonObject& obj)
{
    if (obj.contains("useOverride"))
        useOverride = obj["useOverride"].toBool();

    auto readField = [&](const char* key, auto& target) {
        if (!obj.contains(key)) return;
        QJsonValue v = obj[key];
        QString displayValue;
        if (v.isObject() && v.toObject().contains("value")) {
            displayValue = v.toObject()["value"].toString();
        } else {
            displayValue = v.toVariant().toString();
        }
        target = static_cast<std::decay_t<decltype(target)>>(parseLeadingNumber(displayValue));
    };
    readField("Kind",        kind);
    readField("Domain",      domain);
    readField("Country",     country);
    readField("Category",    category);
    readField("SubCategory", subcategory);
    readField("Specific",    specific);
    readField("Extra",       extra);
}
