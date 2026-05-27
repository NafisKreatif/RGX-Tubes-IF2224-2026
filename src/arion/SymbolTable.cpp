#include "SymbolTable.hpp"
#include <cctype>
#include <iomanip>
#include <sstream>
#include <utility>

using namespace arion;

SymbolTableError::SymbolTableError(const std::string &message) : std::runtime_error(message) {}

SymbolTable::SymbolTable() {
    reset();
}

void SymbolTable::reset() {
    tab_.clear();
    btab_.clear();
    atab_.clear();
    typeDescriptors_.clear();
    blockStack_.clear();
    predefinedLimit_ = 0;

    tab_.push_back(TabEntry{"<nil>", 0, SymbolObjectKind::Reserved, TypeKind::Void, 0, true, 0, 0, "", true});

    BTabEntry global;
    global.name = "<global>";
    global.parent = -1;
    global.lexicalLevel = 0;
    global.kind = BlockKind::Global;
    btab_.push_back(global);
    blockStack_.push_back(0);

    initializePredefinedIdentifiers();
    predefinedLimit_ = static_cast<int>(tab_.size());
}

int SymbolTable::enterBlock(const std::string &name) {
    return enterBlock(name, BlockKind::Anonymous);
}

int SymbolTable::enterBlock(const std::string &name, BlockKind kind) {
    int index = createBlock(name, kind);
    enterBlockByIndex(index);
    return index;
}

void SymbolTable::enterBlockByIndex(int blockIndex) {
    requireBlock(blockIndex);
    blockStack_.push_back(blockIndex);
}

void SymbolTable::leaveBlock() {
    if (blockStack_.size() <= 1) {
        throw SymbolTableError("Cannot leave global block");
    }
    blockStack_.pop_back();
}

int SymbolTable::currentBlockIndex() const {
    return blockStack_.empty() ? -1 : blockStack_.back();
}

int SymbolTable::currentLexicalLevel() const {
    int blockIndex = currentBlockIndex();
    if (!isValidBlockRef(blockIndex)) return 0;
    return btab_[blockIndex].lexicalLevel;
}

int SymbolTable::declare(TabEntry entry) {
    return appendEntry(std::move(entry), true);
}

int SymbolTable::declareProgram(const std::string &name) {
    TabEntry entry;
    entry.identifier = name;
    entry.object = SymbolObjectKind::Program;
    entry.type = TypeKind::Void;
    entry.initialized = true;
    int index = declare(std::move(entry));

    int blockIndex = currentBlockIndex();
    if (isValidBlockRef(blockIndex) && btab_[blockIndex].parent == -1) {
        btab_[blockIndex].name = name;
        btab_[blockIndex].kind = BlockKind::Program;
        btab_[blockIndex].ownerTabIndex = index;
    }
    return index;
}

int SymbolTable::declareConstant(const std::string &name, TypeKind type, std::string value) {
    TabEntry entry;
    entry.identifier = name;
    entry.object = SymbolObjectKind::Constant;
    entry.type = type;
    entry.value = std::move(value);
    entry.initialized = true;
    return declare(std::move(entry));
}

int SymbolTable::declareType(const std::string &name, TypeKind type, int ref) {
    TabEntry entry;
    entry.identifier = name;
    entry.object = SymbolObjectKind::Type;
    entry.type = type;
    entry.ref = ref;
    entry.initialized = true;
    int index = declare(std::move(entry));

    if (type == TypeKind::Record && isValidBlockRef(ref)) {
        btab_[ref].kind = BlockKind::Record;
        btab_[ref].ownerTabIndex = index;
    }
    return index;
}

int SymbolTable::declareVariable(const std::string &name, TypeKind type, int ref) {
    int blockIndex = currentBlockIndex();
    requireBlock(blockIndex);
    BTabEntry &block = btab_[blockIndex];

    TabEntry entry;
    entry.identifier = name;
    entry.object = SymbolObjectKind::Variable;
    entry.type = type;
    entry.ref = ref;
    entry.address = block.variableSize;

    int index = declare(std::move(entry));
    block.variableSize += storageSize(type, ref);
    return index;
}

int SymbolTable::declareParameter(const std::string &name, TypeKind type, int ref, bool normal) {
    int blockIndex = currentBlockIndex();
    requireBlock(blockIndex);
    BTabEntry &block = btab_[blockIndex];

    TabEntry entry;
    entry.identifier = name;
    entry.object = SymbolObjectKind::Parameter;
    entry.type = type;
    entry.ref = ref;
    entry.normal = normal;
    entry.address = block.parameterSize;

    int index = declare(std::move(entry));
    block.parameterSize += normal ? storageSize(type, ref) : 1;
    block.lastParameter = index;
    return index;
}

int SymbolTable::declareField(const std::string &name, TypeKind type, int ref) {
    int blockIndex = currentBlockIndex();
    requireBlock(blockIndex);
    BTabEntry &block = btab_[blockIndex];
    if (block.kind != BlockKind::Record) {
        throw SymbolTableError("Fields can only be declared in a record block: " + name);
    }

    TabEntry entry;
    entry.identifier = name;
    entry.object = SymbolObjectKind::Field;
    entry.type = type;
    entry.ref = ref;
    entry.address = block.variableSize;

    int index = declare(std::move(entry));
    block.variableSize += storageSize(type, ref);
    return index;
}

int SymbolTable::declareProcedure(const std::string &name, int blockRef) {
    TabEntry entry;
    entry.identifier = name;
    entry.object = SymbolObjectKind::Procedure;
    entry.type = TypeKind::Void;
    entry.ref = blockRef;
    entry.initialized = true;
    int index = declare(std::move(entry));

    if (isValidBlockRef(blockRef) && blockRef != 0) {
        btab_[blockRef].kind = BlockKind::Procedure;
        btab_[blockRef].ownerTabIndex = index;
    }
    return index;
}

int SymbolTable::declareFunction(const std::string &name, TypeKind returnType, int blockRef) {
    TabEntry entry;
    entry.identifier = name;
    entry.object = SymbolObjectKind::Function;
    entry.type = returnType;
    entry.ref = blockRef;
    entry.initialized = true;
    int index = declare(std::move(entry));

    if (isValidBlockRef(blockRef) && blockRef != 0) {
        btab_[blockRef].kind = BlockKind::Function;
        btab_[blockRef].returnType = returnType;
        btab_[blockRef].ownerTabIndex = index;
    }
    return index;
}

int SymbolTable::addArray(ATabEntry entry) {
    validateArrayIndexType(entry.indexType);
    if ((entry.indexType == TypeKind::Subrange || entry.indexType == TypeKind::Enumerated) &&
        !isValidTypeDescriptorRef(entry.indexRef)) {
        throw SymbolTableError("Invalid array index type descriptor ref: " + std::to_string(entry.indexRef));
    }
    if (entry.elementType == TypeKind::Array && !isValidArrayRef(entry.elementRef)) {
        throw SymbolTableError("Invalid array element ref: " + std::to_string(entry.elementRef));
    }
    if (entry.elementType == TypeKind::Record && !isValidBlockRef(entry.elementRef)) {
        throw SymbolTableError("Invalid record element ref: " + std::to_string(entry.elementRef));
    }
    if ((entry.elementType == TypeKind::Subrange || entry.elementType == TypeKind::Enumerated) &&
        !isValidTypeDescriptorRef(entry.elementRef)) {
        throw SymbolTableError("Invalid element type descriptor ref: " + std::to_string(entry.elementRef));
    }

    if (entry.indexType == TypeKind::Subrange && isValidTypeDescriptorRef(entry.indexRef)) {
        const TypeDescriptor &range = typeDescriptors_[entry.indexRef];
        if (entry.low.empty()) entry.low = range.low;
        if (entry.high.empty()) entry.high = range.high;
    }

    entry.elementSize = storageSize(entry.elementType, entry.elementRef);
    int lowOrdinal = 0;
    int highOrdinal = 0;
    if (resolveBounds(entry.indexType, entry.indexRef, entry.low, entry.high, lowOrdinal, highOrdinal)) {
        if (lowOrdinal > highOrdinal) {
            throw SymbolTableError("Invalid array bounds: " + entry.low + ".." + entry.high);
        }
        entry.boundsResolved = true;
        entry.lowOrdinal = lowOrdinal;
        entry.highOrdinal = highOrdinal;
        entry.size = (highOrdinal - lowOrdinal + 1) * entry.elementSize;
    }
    else if (entry.size < 0) {
        throw SymbolTableError("Invalid array size for bounds: " + entry.low + ".." + entry.high);
    }

    atab_.push_back(std::move(entry));
    return static_cast<int>(atab_.size()) - 1;
}

int SymbolTable::addArrayType(TypeKind indexType, int indexRef, const std::string &low,
                              const std::string &high, TypeKind elementType, int elementRef) {
    ATabEntry entry;
    entry.indexType = indexType;
    entry.indexRef = indexRef;
    entry.low = low;
    entry.high = high;
    entry.elementType = elementType;
    entry.elementRef = elementRef;
    return addArray(std::move(entry));
}

int SymbolTable::declareArrayType(const std::string &name, TypeKind indexType, int indexRef,
                                  const std::string &low, const std::string &high,
                                  TypeKind elementType, int elementRef) {
    ensureDeclarableIdentifier(name);
    int arrayRef = addArrayType(indexType, indexRef, low, high, elementType, elementRef);
    return declareType(name, TypeKind::Array, arrayRef);
}

int SymbolTable::addBlock(BTabEntry entry) {
    if (entry.parent >= static_cast<int>(btab_.size())) {
        throw SymbolTableError("Invalid parent block ref: " + std::to_string(entry.parent));
    }
    if (entry.parent >= 0 && entry.lexicalLevel == 0) {
        entry.lexicalLevel = btab_[entry.parent].lexicalLevel + 1;
    }

    btab_.push_back(std::move(entry));
    return static_cast<int>(btab_.size()) - 1;
}

int SymbolTable::createBlock(const std::string &name, BlockKind kind, int parentBlock) {
    int parent = parentBlock == -2 ? currentBlockIndex() : parentBlock;
    if (parent >= 0) requireBlock(parent);

    BTabEntry block;
    block.name = name;
    block.parent = parent;
    block.kind = kind;
    block.lexicalLevel = parent >= 0 ? btab_[parent].lexicalLevel + 1 : 0;
    return addBlock(std::move(block));
}

int SymbolTable::createRecordBlock(const std::string &name) {
    return createBlock(name, BlockKind::Record);
}

int SymbolTable::beginRecordType(const std::string &name) {
    int blockRef = createRecordBlock(name);
    enterBlockByIndex(blockRef);
    return blockRef;
}

void SymbolTable::endRecordType() {
    int blockIndex = currentBlockIndex();
    const BTabEntry &block = requireBlock(blockIndex);
    if (block.kind != BlockKind::Record) {
        throw SymbolTableError("Current block is not a record block: " + block.name);
    }
    leaveBlock();
}

int SymbolTable::declareRecordType(const std::string &name, int recordBlockRef) {
    ensureDeclarableIdentifier(name);
    const BTabEntry &block = requireBlock(recordBlockRef);
    if (block.kind != BlockKind::Record) {
        throw SymbolTableError("Block is not a record block for type: " + name);
    }
    return declareType(name, TypeKind::Record, recordBlockRef);
}

int SymbolTable::createProcedureBlock(const std::string &name) {
    return createBlock(name, BlockKind::Procedure);
}

int SymbolTable::createFunctionBlock(const std::string &name) {
    return createBlock(name, BlockKind::Function);
}

int SymbolTable::declareProcedureWithBlock(const std::string &name) {
    ensureDeclarableIdentifier(name);
    int blockRef = createProcedureBlock(name);
    return declareProcedure(name, blockRef);
}

int SymbolTable::declareFunctionWithBlock(const std::string &name, TypeKind returnType, int returnRef) {
    ensureDeclarableIdentifier(name);
    int blockRef = createFunctionBlock(name);
    btab_[blockRef].returnType = returnType;
    btab_[blockRef].returnRef = returnRef;
    return declareFunction(name, returnType, blockRef);
}

int SymbolTable::addSubrange(TypeKind baseType, const std::string &low, const std::string &high, int baseRef) {
    validateSubrangeBase(baseType);

    TypeDescriptor descriptor;
    descriptor.kind = TypeDescriptorKind::Subrange;
    descriptor.baseType = baseType;
    descriptor.baseRef = baseRef;
    descriptor.low = low;
    descriptor.high = high;
    descriptor.size = 1;

    int lowOrdinal = 0;
    int highOrdinal = 0;
    if (resolveBounds(baseType, baseRef, low, high, lowOrdinal, highOrdinal)) {
        if (lowOrdinal > highOrdinal) {
            throw SymbolTableError("Invalid subrange bounds: " + low + ".." + high);
        }
        descriptor.boundsResolved = true;
        descriptor.lowOrdinal = lowOrdinal;
        descriptor.highOrdinal = highOrdinal;
    }

    typeDescriptors_.push_back(std::move(descriptor));
    return static_cast<int>(typeDescriptors_.size()) - 1;
}

int SymbolTable::declareSubrangeType(const std::string &name, TypeKind baseType,
                                     const std::string &low, const std::string &high, int baseRef) {
    ensureDeclarableIdentifier(name);
    int ref = addSubrange(baseType, low, high, baseRef);
    return declareType(name, TypeKind::Subrange, ref);
}

int SymbolTable::addEnumeratedType(const std::vector<std::string> &values) {
    if (values.empty()) {
        throw SymbolTableError("Enumerated type must contain at least one value");
    }

    TypeDescriptor descriptor;
    descriptor.kind = TypeDescriptorKind::Enumerated;
    descriptor.baseType = TypeKind::Enumerated;
    descriptor.low = "0";
    descriptor.high = std::to_string(static_cast<int>(values.size()) - 1);
    descriptor.boundsResolved = true;
    descriptor.lowOrdinal = 0;
    descriptor.highOrdinal = static_cast<int>(values.size()) - 1;
    descriptor.values = values;
    descriptor.size = 1;

    for (std::size_t i = 0; i < values.size(); ++i) {
        for (std::size_t j = i + 1; j < values.size(); ++j) {
            if (sameIdentifier(values[i], values[j])) {
                throw SymbolTableError("Duplicate enumerated value: " + values[i]);
            }
        }
    }

    typeDescriptors_.push_back(std::move(descriptor));
    return static_cast<int>(typeDescriptors_.size()) - 1;
}

int SymbolTable::declareEnumeratedType(const std::string &name, const std::vector<std::string> &values) {
    int nonRecordBlock = currentBlockIndex();
    while (btab_[nonRecordBlock].kind == BlockKind::Record) {
        nonRecordBlock = btab_[nonRecordBlock].parent;
    }
    enterBlockByIndex(nonRecordBlock);
    ensureDeclarableIdentifier(name);
    for (const std::string &value : values) {
        if (sameIdentifier(name, value)) {
            throw SymbolTableError("Enumerated value conflicts with type name: " + value);
        }
        ensureDeclarableIdentifier(value);
    }
    leaveBlock();

    int ref = addEnumeratedType(values);
    int typeIndex = declareType(name, TypeKind::Enumerated, ref);
    for (std::size_t ordinal = 0; ordinal < values.size(); ++ordinal) {
        declareEnumeratedConstant(values[ordinal], ref, static_cast<int>(ordinal));
    }
    return typeIndex;
}

int SymbolTable::declareEnumeratedConstant(const std::string &name, int enumeratedRef, int ordinal) {
    const TypeDescriptor &descriptor = requireTypeDescriptor(enumeratedRef);
    if (descriptor.kind != TypeDescriptorKind::Enumerated) {
        throw SymbolTableError("Type descriptor is not enumerated for constant: " + name);
    }
    if (ordinal < 0 || ordinal >= static_cast<int>(descriptor.values.size())) {
        throw SymbolTableError("Invalid enumerated ordinal for constant: " + name);
    }
    if (!sameIdentifier(name, descriptor.values[ordinal])) {
        throw SymbolTableError("Enumerated constant does not match descriptor ordinal: " + name);
    }

    TabEntry entry;
    entry.identifier = name;
    entry.object = SymbolObjectKind::Constant;
    entry.type = TypeKind::Enumerated;
    entry.ref = enumeratedRef;
    entry.address = ordinal;
    entry.value = std::to_string(ordinal);
    entry.initialized = true;
    return declare(std::move(entry));
}

int SymbolTable::lookupIndex(const std::string &identifier) const {
    int blockIndex = currentBlockIndex();
    while (blockIndex >= 0 && isValidBlockRef(blockIndex)) {
        int tabIndex = btab_[blockIndex].last;
        while (tabIndex > 0 && isValidTabIndex(tabIndex)) {
            if (sameIdentifier(tab_[tabIndex].identifier, identifier)) return tabIndex;
            tabIndex = tab_[tabIndex].link;
        }
        blockIndex = btab_[blockIndex].parent;
    }

    for (int i = predefinedLimit_ - 1; i > 0; --i) {
        if (sameIdentifier(tab_[i].identifier, identifier)) {
            return i;
        }
    }
    return -1;
}

int SymbolTable::lookupCurrentScopeIndex(const std::string &identifier) const {
    int tabIndex = currentBlockLast();
    while (tabIndex > 0 && isValidTabIndex(tabIndex)) {
        if (sameIdentifier(tab_[tabIndex].identifier, identifier)) return tabIndex;
        tabIndex = tab_[tabIndex].link;
    }
    return -1;
}

const TabEntry *SymbolTable::lookup(const std::string &identifier) const {
    int index = lookupIndex(identifier);
    return index >= 0 ? &tab_[index] : nullptr;
}

const TabEntry *SymbolTable::lookupCurrentScope(const std::string &identifier) const {
    int index = lookupCurrentScopeIndex(identifier);
    return index >= 0 ? &tab_[index] : nullptr;
}

int SymbolTable::requireLookupIndex(const std::string &identifier) const {
    int index = lookupIndex(identifier);
    if (index < 0) {
        throw SymbolTableError("Undefined identifier: " + identifier);
    }
    return index;
}

const TabEntry &SymbolTable::requireLookup(const std::string &identifier) const {
    return tab_[requireLookupIndex(identifier)];
}

int SymbolTable::requireTypeIndex(const std::string &identifier) const {
    int index = requireLookupIndex(identifier);
    if (tab_[index].object != SymbolObjectKind::Type) {
        throw SymbolTableError("Identifier is not a type: " + identifier);
    }
    return index;
}

const TabEntry &SymbolTable::requireType(const std::string &identifier) const {
    return tab_[requireTypeIndex(identifier)];
}

const ATabEntry &SymbolTable::requireArray(int ref) const {
    if (!isValidArrayRef(ref)) {
        throw SymbolTableError("Invalid array ref: " + std::to_string(ref));
    }
    return atab_[ref];
}

const BTabEntry &SymbolTable::requireBlock(int ref) const {
    if (!isValidBlockRef(ref)) {
        throw SymbolTableError("Invalid block ref: " + std::to_string(ref));
    }
    return btab_[ref];
}

const TypeDescriptor &SymbolTable::requireTypeDescriptor(int ref) const {
    if (!isValidTypeDescriptorRef(ref)) {
        throw SymbolTableError("Invalid type descriptor ref: " + std::to_string(ref));
    }
    return typeDescriptors_[ref];
}

int SymbolTable::typeSize(TypeKind type, int ref) const {
    switch (type) {
        case TypeKind::Integer:
        case TypeKind::Real:
        case TypeKind::Boolean:
        case TypeKind::Char:
        case TypeKind::String:
        case TypeKind::Subrange:
        case TypeKind::Enumerated:
            return 1;
        case TypeKind::Array:
            if (isValidArrayRef(ref) && atab_[ref].size > 0) return atab_[ref].size;
            return 1;
        case TypeKind::Record:
            if (isValidBlockRef(ref) && btab_[ref].variableSize > 0) return btab_[ref].variableSize;
            return 1;
        case TypeKind::Void:
        case TypeKind::Unknown:
            return 0;
    }
    return 0;
}

const std::vector<TabEntry> &SymbolTable::tab() const {
    return tab_;
}

const std::vector<BTabEntry> &SymbolTable::btab() const {
    return btab_;
}

const std::vector<ATabEntry> &SymbolTable::atab() const {
    return atab_;
}

const std::vector<TypeDescriptor> &SymbolTable::typeDescriptors() const {
    return typeDescriptors_;
}

std::string SymbolTable::dumpTab() const {
    std::ostringstream out;

    int tabIdxWidth = maxTabIdxWidth();
    int tabIdWidth = maxTabIdWidth();
    int tabObjWidth = maxTabObjWidth();
    int tabTypeWidth = maxTabTypeWidth();
    int tabRefWidth = maxTabRefWidth();
    int tabNrmWidth = maxTabNrmWidth();
    int tabLvlWidth = maxTabLvlWidth();
    int tabAdrWidth = maxTabAdrWidth();
    int tabLinkWidth = maxTabLinkWidth();
    int tabValueWidth = maxTabValueWidth();
    int tabInitWidth = maxTabInitWidth();

    out << " "
        << std::setw(tabIdxWidth) << "idx" << " | "
        << std::setw(tabIdWidth) << "identifier" << " | "
        << std::setw(tabObjWidth) << "object" << " | "
        << std::setw(tabTypeWidth) << "type" << " | "
        << std::setw(tabRefWidth) << "ref" << " | "
        << std::setw(tabNrmWidth) << "nrm" << " | "
        << std::setw(tabLvlWidth) << "lvl" << " | "
        << std::setw(tabAdrWidth) << "adr" << " | "
        << std::setw(tabLinkWidth) << "link" << " | "
        << std::setw(tabValueWidth) << "value" << " | "
        << std::setw(tabInitWidth) << "initialized" << " \n";

    out << std::setfill('-') << "-"
        << std::setw(tabIdxWidth) << "" << "-|-"
        << std::setw(tabIdWidth) << "" << "-|-"
        << std::setw(tabObjWidth) << "" << "-|-"
        << std::setw(tabTypeWidth) << "" << "-|-"
        << std::setw(tabRefWidth) << "" << "-|-"
        << std::setw(tabNrmWidth) << "" << "-|-"
        << std::setw(tabLvlWidth) << "" << "-|-"
        << std::setw(tabAdrWidth) << "" << "-|-"
        << std::setw(tabLinkWidth) << "" << "-|-"
        << std::setw(tabValueWidth) << "" << "-|-"
        << std::setw(tabInitWidth) << "" << "-\n";

    out << std::setfill(' ');
    for (std::size_t i = 0; i < tab_.size(); ++i) {
        const TabEntry &entry = tab_[i];
        out << " "
            << std::setw(tabIdxWidth) << i << " | "
            << std::setw(tabIdWidth) << entry.identifier << " | "
            << std::setw(tabObjWidth) << objectKindToString(entry.object) << " | "
            << std::setw(tabTypeWidth) << typeKindToString(entry.type) << " | "
            << std::setw(tabRefWidth) << entry.ref << " | "
            << std::setw(tabNrmWidth) << (entry.normal ? 1 : 0) << " | "
            << std::setw(tabLvlWidth) << entry.lexicalLevel << " | "
            << std::setw(tabAdrWidth) << entry.address << " | "
            << std::setw(tabLinkWidth) << entry.link << " | "
            << std::setw(tabValueWidth) << entry.value << " | "
            << std::setw(tabInitWidth) << (entry.initialized ? 1 : 0) << " \n";
    }
    return out.str();
}

std::string SymbolTable::dumpBTab() const {
    std::ostringstream out;

    int bTabIdxWidth = maxBTabIdxWidth();
    int bTabNameWidth = maxBTabNameWidth();
    int bTabKindWidth = maxBTabKindWidth();
    int bTabParentWidth = maxBTabParentWidth();
    int bTabLastWidth = maxBTabLastWidth();
    int bTabLParWidth = maxBTabLParWidth();
    int bTabPSizeWidth = maxBTabPSizeWidth();
    int bTabVSizeWidth = maxBTabVSizeWidth();
    int bTabLvlWidth = maxBTabLvlWidth();
    int bTabReturnTypeWidth = maxBTabReturnTypeWidth();
    int bTabReturnRefWidth = maxBTabReturnRefWidth();
    int bTabOwnerWidth = maxBTabOwnerWidth();

    out << " "
        << std::setw(bTabIdxWidth) << "idx" << " | "
        << std::setw(bTabNameWidth) << "name" << " | "
        << std::setw(bTabKindWidth) << "kind" << " | "
        << std::setw(bTabParentWidth) << "parent" << " | "
        << std::setw(bTabLastWidth) << "last" << " | "
        << std::setw(bTabLParWidth) << "lpar" << " | "
        << std::setw(bTabPSizeWidth) << "pSize" << " | "
        << std::setw(bTabVSizeWidth) << "vSize" << " | "
        << std::setw(bTabLvlWidth) << "lvl" << " | "
        << std::setw(bTabReturnTypeWidth) << "returnType" << " | "
        << std::setw(bTabReturnRefWidth) << "returnRef" << " | "
        << std::setw(bTabOwnerWidth) << "owner" << " \n";

    out << std::setfill('-') << "-"
        << std::setw(bTabIdxWidth) << "" << "-|-"
        << std::setw(bTabNameWidth) << "" << "-|-"
        << std::setw(bTabKindWidth) << "" << "-|-"
        << std::setw(bTabParentWidth) << "" << "-|-"
        << std::setw(bTabLastWidth) << "" << "-|-"
        << std::setw(bTabLParWidth) << "" << "-|-"
        << std::setw(bTabPSizeWidth) << "" << "-|-"
        << std::setw(bTabVSizeWidth) << "" << "-|-"
        << std::setw(bTabLvlWidth) << "" << "-|-"
        << std::setw(bTabReturnTypeWidth) << "" << "-|-"
        << std::setw(bTabReturnRefWidth) << "" << "-|-"
        << std::setw(bTabOwnerWidth) << "" << "-\n";

    out << std::setfill(' ');

    for (std::size_t i = 0; i < btab_.size(); ++i) {
        const BTabEntry &entry = btab_[i];
        out << " "
            << std::setw(bTabIdxWidth) << i << " | "
            << std::setw(bTabNameWidth) << entry.name << " | "
            << std::setw(bTabKindWidth) << blockKindToString(entry.kind) << " | "
            << std::setw(bTabParentWidth) << entry.parent << " | "
            << std::setw(bTabLastWidth) << entry.last << " | "
            << std::setw(bTabLParWidth) << entry.lastParameter << " | "
            << std::setw(bTabPSizeWidth) << entry.parameterSize << " | "
            << std::setw(bTabVSizeWidth) << entry.variableSize << " | "
            << std::setw(bTabLvlWidth) << entry.lexicalLevel << " | "
            << std::setw(bTabReturnTypeWidth) << typeKindToString(entry.returnType) << " | "
            << std::setw(bTabReturnRefWidth) << entry.returnRef << " | "
            << std::setw(bTabOwnerWidth) << entry.ownerTabIndex << " \n";
    }
    return out.str();
}

std::string SymbolTable::dumpATab() const {
    std::ostringstream out;
    int atabIdxWidth = maxATabIdxWidth();
    int atabIdxTypeWidth = maxATabIdxTypeWidth();
    int atabIdxRefWidth = maxATabIdxRefWidth();
    int atabElTypeWidth = maxATabElTypeWidth();
    int atabElRefWidth = maxATabElRefWidth();
    int atabLowWidth = maxATabLowWidth();
    int atabHighWidth = maxATabHighWidth();
    int atabResolvedWidth = maxATabResolvedWidth();
    int atabLowOrdWidth = maxATabLowOrdWidth();
    int atabHighOrdWidth = maxATabHighOrdWidth();
    int atabElSizeWidth = maxATabElSizeWidth();
    int atabSizeWidth = maxATabSizeWidth();

    out << " "
        << std::setw(atabIdxWidth) << "idx" << " | "
        << std::setw(atabIdxTypeWidth) << "idxType" << " | "
        << std::setw(atabIdxRefWidth) << "idxRef" << " | "
        << std::setw(atabElTypeWidth) << "elType" << " | "
        << std::setw(atabElRefWidth) << "elRef" << " | "
        << std::setw(atabLowWidth) << "low" << " | "
        << std::setw(atabHighWidth) << "high" << " | "
        << std::setw(atabResolvedWidth) << "resolved" << " | "
        << std::setw(atabLowOrdWidth) << "lowOrd" << " | "
        << std::setw(atabHighOrdWidth) << "highOrd" << " | "
        << std::setw(atabElSizeWidth) << "elSize" << " | "
        << std::setw(atabSizeWidth) << "size" << " \n";

    out << std::setfill('-') << "-"
        << std::setw(atabIdxWidth) << "" << "-|-"
        << std::setw(atabIdxTypeWidth) << "" << "-|-"
        << std::setw(atabIdxRefWidth) << "" << "-|-"
        << std::setw(atabElTypeWidth) << "" << "-|-"
        << std::setw(atabElRefWidth) << "" << "-|-"
        << std::setw(atabLowWidth) << "" << "-|-"
        << std::setw(atabHighWidth) << "" << "-|-"
        << std::setw(atabResolvedWidth) << "" << "-|-"
        << std::setw(atabLowOrdWidth) << "" << "-|-"
        << std::setw(atabHighOrdWidth) << "" << "-|-"
        << std::setw(atabElSizeWidth) << "" << "-|-"
        << std::setw(atabSizeWidth) << "" << "-\n";

    out << std::setfill(' ');

    for (std::size_t i = 0; i < atab_.size(); ++i) {
        const ATabEntry &entry = atab_[i];
        out << " "
            << std::setw(atabIdxWidth) << i << " | "
            << std::setw(atabIdxTypeWidth) << typeKindToString(entry.indexType) << " | "
            << std::setw(atabIdxRefWidth) << entry.indexRef << " | "
            << std::setw(atabElTypeWidth) << typeKindToString(entry.elementType) << " | "
            << std::setw(atabElRefWidth) << entry.elementRef << " | "
            << std::setw(atabLowWidth) << entry.low << " | "
            << std::setw(atabHighWidth) << entry.high << " | "
            << std::setw(atabResolvedWidth) << (entry.boundsResolved ? 1 : 0) << " | "
            << std::setw(atabLowOrdWidth) << entry.lowOrdinal << " | "
            << std::setw(atabHighOrdWidth) << entry.highOrdinal << " | "
            << std::setw(atabElSizeWidth) << entry.elementSize << " | "
            << std::setw(atabSizeWidth) << entry.size << " \n";
    }
    return out.str();
}

std::string SymbolTable::dumpTypeDescriptors() const {
    std::ostringstream out;

    int typeDescriptorIdxWidth = maxTypeDescriptorIdxWidth();
    int typeDescriptorKindWidth = maxTypeDescriptorKindWidth();
    int typeDescriptorBaseTypeWidth = maxTypeDescriptorBaseTypeWidth();
    int typeDescriptorBaseRefWidth = maxTypeDescriptorBaseRefWidth();
    int typeDescriptorLowWidth = maxTypeDescriptorLowWidth();
    int typeDescriptorHighWidth = maxTypeDescriptorHighWidth();
    int typeDescriptorResolvedWidth = maxTypeDescriptorResolvedWidth();
    int typeDescriptorLowOrdWidth = maxTypeDescriptorLowOrdWidth();
    int typeDescriptorHighOrdWidth = maxTypeDescriptorHighOrdWidth();
    int typeDescriptorSizeWidth = maxTypeDescriptorSizeWidth();
    int typeDescriptorValuesWidth = maxTypeDescriptorValuesWidth();

    out << " "
        << std::setw(typeDescriptorIdxWidth) << "idx" << " | "
        << std::setw(typeDescriptorKindWidth) << "kind" << " | "
        << std::setw(typeDescriptorBaseTypeWidth) << "baseType" << " | "
        << std::setw(typeDescriptorBaseRefWidth) << "baseRef" << " | "
        << std::setw(typeDescriptorLowWidth) << "low" << " | "
        << std::setw(typeDescriptorHighWidth) << "high" << " | "
        << std::setw(typeDescriptorResolvedWidth) << "resolved" << " | "
        << std::setw(typeDescriptorLowOrdWidth) << "lowOrd" << " | "
        << std::setw(typeDescriptorHighOrdWidth) << "highOrd" << " | "
        << std::setw(typeDescriptorSizeWidth) << "size" << " | "
        << std::setw(typeDescriptorValuesWidth) << "values" << " \n";

    out << std::setfill('-') << "-"
        << std::setw(typeDescriptorIdxWidth) << "" << "-|-"
        << std::setw(typeDescriptorKindWidth) << "" << "-|-"
        << std::setw(typeDescriptorBaseTypeWidth) << "" << "-|-"
        << std::setw(typeDescriptorBaseRefWidth) << "" << "-|-"
        << std::setw(typeDescriptorLowWidth) << "" << "-|-"
        << std::setw(typeDescriptorHighWidth) << "" << "-|-"
        << std::setw(typeDescriptorResolvedWidth) << "" << "-|-"
        << std::setw(typeDescriptorLowOrdWidth) << "" << "-|-"
        << std::setw(typeDescriptorHighOrdWidth) << "" << "-|-"
        << std::setw(typeDescriptorSizeWidth) << "" << "-|-"
        << std::setw(typeDescriptorValuesWidth) << "" << "-\n";

    out << std::setfill(' ');

    for (std::size_t i = 0; i < typeDescriptors_.size(); ++i) {
        const TypeDescriptor &entry = typeDescriptors_[i];
        out << " "
            << std::setw(typeDescriptorIdxWidth) << i << " | "
            << std::setw(typeDescriptorKindWidth) << descriptorKindToString(entry.kind) << " | "
            << std::setw(typeDescriptorBaseTypeWidth) << typeKindToString(entry.baseType) << " | "
            << std::setw(typeDescriptorBaseRefWidth) << entry.baseRef << " | "
            << std::setw(typeDescriptorLowWidth) << entry.low << " | "
            << std::setw(typeDescriptorHighWidth) << entry.high << " | "
            << std::setw(typeDescriptorResolvedWidth) << (entry.boundsResolved ? 1 : 0) << " | "
            << std::setw(typeDescriptorLowOrdWidth) << entry.lowOrdinal << " | "
            << std::setw(typeDescriptorHighOrdWidth) << entry.highOrdinal << " | "
            << std::setw(typeDescriptorSizeWidth) << entry.size << " | "
            << std::setw(typeDescriptorValuesWidth) << descriptorValuesToString(entry) << " \n";
    }
    return out.str();
}

std::string SymbolTable::objectKindToString(SymbolObjectKind kind) {
    switch (kind) {
        case SymbolObjectKind::Reserved:
            return "reserved";
        case SymbolObjectKind::Program:
            return "program";
        case SymbolObjectKind::Constant:
            return "constant";
        case SymbolObjectKind::Type:
            return "type";
        case SymbolObjectKind::Variable:
            return "variable";
        case SymbolObjectKind::Procedure:
            return "procedure";
        case SymbolObjectKind::Function:
            return "function";
        case SymbolObjectKind::Parameter:
            return "parameter";
        case SymbolObjectKind::Field:
            return "field";
        case SymbolObjectKind::Unknown:
            return "unknown";
    }
    return "unknown";
}

std::string SymbolTable::typeKindToString(TypeKind kind) {
    switch (kind) {
        case TypeKind::Unknown:
            return "unknown";
        case TypeKind::Void:
            return "void";
        case TypeKind::Integer:
            return "integer";
        case TypeKind::Real:
            return "real";
        case TypeKind::Boolean:
            return "boolean";
        case TypeKind::Char:
            return "char";
        case TypeKind::String:
            return "string";
        case TypeKind::Subrange:
            return "subrange";
        case TypeKind::Array:
            return "array";
        case TypeKind::Record:
            return "record";
        case TypeKind::Enumerated:
            return "enumerated";
    }
    return "unknown";
}

std::string SymbolTable::blockKindToString(BlockKind kind) {
    switch (kind) {
        case BlockKind::Global:
            return "global";
        case BlockKind::Program:
            return "program";
        case BlockKind::Procedure:
            return "procedure";
        case BlockKind::Function:
            return "function";
        case BlockKind::Record:
            return "record";
        case BlockKind::Anonymous:
            return "anonymous";
    }
    return "anonymous";
}

std::string SymbolTable::descriptorKindToString(TypeDescriptorKind kind) {
    switch (kind) {
        case TypeDescriptorKind::Unknown:
            return "unknown";
        case TypeDescriptorKind::Subrange:
            return "subrange";
        case TypeDescriptorKind::Enumerated:
            return "enumerated";
    }
    return "unknown";
}

void SymbolTable::initializePredefinedIdentifiers() {
    const std::vector<std::pair<std::string, TypeKind>> reservedTypes = {
        {"and", TypeKind::Unknown},
        {"array", TypeKind::Unknown},
        {"begin", TypeKind::Unknown},
        {"case", TypeKind::Unknown},
        {"const", TypeKind::Unknown},
        {"div", TypeKind::Unknown},
        {"downto", TypeKind::Unknown},
        {"do", TypeKind::Unknown},
        {"else", TypeKind::Unknown},
        {"end", TypeKind::Unknown},
        {"for", TypeKind::Unknown},
        {"function", TypeKind::Unknown},
        {"if", TypeKind::Unknown},
        {"mod", TypeKind::Unknown},
        {"not", TypeKind::Unknown},
        {"of", TypeKind::Unknown},
        {"or", TypeKind::Unknown},
        {"procedure", TypeKind::Unknown},
        {"program", TypeKind::Unknown},
        {"record", TypeKind::Unknown},
        {"repeat", TypeKind::Unknown},
        {"integer", TypeKind::Integer},
        {"real", TypeKind::Real},
        {"boolean", TypeKind::Boolean},
        {"char", TypeKind::Char},
        {"string", TypeKind::String},
        {"then", TypeKind::Unknown},
        {"to", TypeKind::Unknown},
        {"type", TypeKind::Unknown},
        {"until", TypeKind::Unknown},
        {"var", TypeKind::Unknown},
        {"while", TypeKind::Unknown},
    };

    for (const auto &reserved : reservedTypes) {
        TabEntry entry;
        entry.identifier = reserved.first;
        entry.object = reserved.second == TypeKind::Unknown ? SymbolObjectKind::Reserved : SymbolObjectKind::Type;
        entry.type = reserved.second;
        entry.initialized = true;
        appendEntry(std::move(entry), false);
    }

    appendEntry(TabEntry{"true", 0, SymbolObjectKind::Constant, TypeKind::Boolean, 0, true, 0, 1, "true", true}, false);
    appendEntry(TabEntry{"false", 0, SymbolObjectKind::Constant, TypeKind::Boolean, 0, true, 0, 0, "false", true}, false);
    appendEntry(TabEntry{"readln", 0, SymbolObjectKind::Procedure, TypeKind::Void, 0, true, 0, 0, "", true}, false);

    auto appendWriteLikeProcedure = [this](const std::string &name) {
        int blockRef = createProcedureBlock(name);
        int tabIndex = appendEntry(TabEntry{name, 0, SymbolObjectKind::Procedure, TypeKind::Void,
                                            blockRef, true, 0, 0, "", true}, false);
        btab_[blockRef].ownerTabIndex = tabIndex;

        enterBlockByIndex(blockRef);
        declareParameter("<" + name + "-arg>", TypeKind::String);
        leaveBlock();
    };

    appendWriteLikeProcedure("writeln");
    appendWriteLikeProcedure("write");
}

int SymbolTable::appendEntry(TabEntry entry, bool updateCurrentBlock) {
    if (updateCurrentBlock) {
        ensureDeclarableIdentifier(entry.identifier);
    }

    entry.lexicalLevel = updateCurrentBlock ? currentLexicalLevel() : 0;
    entry.link = updateCurrentBlock ? currentBlockLast() : 0;

    tab_.push_back(std::move(entry));
    int index = static_cast<int>(tab_.size()) - 1;
    if (updateCurrentBlock) {
        setCurrentBlockLast(index);
    }
    return index;
}

int SymbolTable::lookupPredefinedIndex(const std::string &identifier) const {
    for (int i = predefinedLimit_ - 1; i > 0; --i) {
        if (sameIdentifier(tab_[i].identifier, identifier)) return i;
    }
    return -1;
}

int SymbolTable::currentBlockLast() const {
    int blockIndex = currentBlockIndex();
    if (!isValidBlockRef(blockIndex)) return 0;
    return btab_[blockIndex].last;
}

void SymbolTable::setCurrentBlockLast(int tabIndex) {
    int blockIndex = currentBlockIndex();
    if (!isValidBlockRef(blockIndex)) {
        throw SymbolTableError("No active block for declaration");
    }
    btab_[blockIndex].last = tabIndex;
}

std::string SymbolTable::normalize(const std::string &identifier) const {
    std::string normalized = identifier;
    for (char &c : normalized) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return normalized;
}

bool SymbolTable::sameIdentifier(const std::string &left, const std::string &right) const {
    return normalize(left) == normalize(right);
}

bool SymbolTable::isValidTabIndex(int index) const {
    return index >= 0 && index < static_cast<int>(tab_.size());
}

bool SymbolTable::isValidArrayRef(int ref) const {
    return ref >= 0 && ref < static_cast<int>(atab_.size());
}

bool SymbolTable::isValidBlockRef(int ref) const {
    return ref >= 0 && ref < static_cast<int>(btab_.size());
}

bool SymbolTable::isValidTypeDescriptorRef(int ref) const {
    return ref >= 0 && ref < static_cast<int>(typeDescriptors_.size());
}

void SymbolTable::ensureDeclarableIdentifier(const std::string &identifier) const {
    if (identifier.empty()) {
        throw SymbolTableError("Identifier cannot be empty");
    }
    if (lookupCurrentScope(identifier) != nullptr) {
        throw SymbolTableError("Redeclaration of identifier in the same scope: " + identifier);
    }
    if (lookupPredefinedIndex(identifier) >= 0) {
        throw SymbolTableError("Identifier conflicts with reserved/predefined identifier: " + identifier);
    }
}

void SymbolTable::validateSubrangeBase(TypeKind baseType) const {
    if (baseType == TypeKind::Real) {
        throw SymbolTableError("Real cannot be used as a subrange base type");
    }
    if (baseType != TypeKind::Integer &&
        baseType != TypeKind::Char &&
        baseType != TypeKind::Boolean &&
        baseType != TypeKind::Enumerated) {
        throw SymbolTableError("Invalid subrange base type: " + typeKindToString(baseType));
    }
}

void SymbolTable::validateArrayIndexType(TypeKind indexType) const {
    if (indexType == TypeKind::Real) {
        throw SymbolTableError("Real cannot be used as an array index type");
    }
    if (indexType != TypeKind::Integer &&
        indexType != TypeKind::Char &&
        indexType != TypeKind::Boolean &&
        indexType != TypeKind::Subrange &&
        indexType != TypeKind::Enumerated) {
        throw SymbolTableError("Invalid array index type: " + typeKindToString(indexType));
    }
}

bool SymbolTable::parseIntegerLiteral(const std::string &text, int &value) const {
    if (text.empty()) return false;

    std::size_t pos = 0;
    if (text[pos] == '+' || text[pos] == '-') ++pos;
    if (pos == text.size()) return false;
    for (; pos < text.size(); ++pos) {
        if (!std::isdigit(static_cast<unsigned char>(text[pos]))) return false;
    }

    try {
        value = std::stoi(text);
        return true;
    } catch (...) {
        return false;
    }
}

bool SymbolTable::ordinalFromLiteral(TypeKind type, int ref, const std::string &text, int &ordinal) const {
    if (type == TypeKind::Integer) {
        return parseIntegerLiteral(text, ordinal);
    }
    if (type == TypeKind::Boolean) {
        std::string lowered = normalize(text);
        if (lowered == "false") {
            ordinal = 0;
            return true;
        }
        if (lowered == "true") {
            ordinal = 1;
            return true;
        }
        return false;
    }
    if (type == TypeKind::Char) {
        if (text.size() == 1) {
            ordinal = static_cast<unsigned char>(text[0]);
            return true;
        }
        if (text.size() >= 3 && text.front() == '\'' && text.back() == '\'') {
            ordinal = static_cast<unsigned char>(text[1]);
            return true;
        }
        return false;
    }
    if (type == TypeKind::Enumerated) {
        if (!isValidTypeDescriptorRef(ref)) return false;
        const TypeDescriptor &descriptor = typeDescriptors_[ref];
        if (descriptor.kind != TypeDescriptorKind::Enumerated) return false;
        for (std::size_t i = 0; i < descriptor.values.size(); ++i) {
            if (sameIdentifier(descriptor.values[i], text)) {
                ordinal = static_cast<int>(i);
                return true;
            }
        }
        return false;
    }
    if (type == TypeKind::Subrange) {
        if (!isValidTypeDescriptorRef(ref)) return false;
        const TypeDescriptor &descriptor = typeDescriptors_[ref];
        if (descriptor.kind != TypeDescriptorKind::Subrange) return false;
        return ordinalFromLiteral(descriptor.baseType, descriptor.baseRef, text, ordinal);
    }
    return false;
}

bool SymbolTable::resolveBounds(TypeKind type, int ref, const std::string &low,
                                const std::string &high, int &lowOrdinal, int &highOrdinal) const {
    if (type == TypeKind::Subrange && isValidTypeDescriptorRef(ref)) {
        const TypeDescriptor &descriptor = typeDescriptors_[ref];
        if (descriptor.kind == TypeDescriptorKind::Subrange && low.empty() && high.empty()) {
            if (!descriptor.boundsResolved) return false;
            lowOrdinal = descriptor.lowOrdinal;
            highOrdinal = descriptor.highOrdinal;
            return true;
        }
    }

    if (low.empty() || high.empty()) return false;
    return ordinalFromLiteral(type, ref, low, lowOrdinal) &&
           ordinalFromLiteral(type, ref, high, highOrdinal);
}

int SymbolTable::storageSize(TypeKind type, int ref) const {
    int size = typeSize(type, ref);
    return size > 0 ? size : 1;
}

std::string SymbolTable::descriptorValuesToString(const TypeDescriptor &descriptor) const {
    if (descriptor.values.empty()) return "-";

    std::ostringstream out;
    for (std::size_t i = 0; i < descriptor.values.size(); ++i) {
        if (i > 0) out << ',';
        out << descriptor.values[i];
    }
    return out.str();
}

static int digitCount(int n) {
    int count = 1;
    if (n < 0) {
        n = -n;
        count++;
    }
    while (n >= 10) {
        count++;
        n /= 10;
    }
    return count;
}

int SymbolTable::maxTabIdxWidth() const {
    return std::max(3, digitCount(tab_.size()));
}

int SymbolTable::maxTabIdWidth() const {
    int maxLength = 12;
    for (auto &&entry : tab_) {
        maxLength = std::max((int)entry.identifier.size(), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxTabObjWidth() const {
    return 11;
}
int SymbolTable::maxTabTypeWidth() const {
    return 11;
}
int SymbolTable::maxTabRefWidth() const {
    int maxLength = 3;
    for (auto &&entry : tab_) {
        maxLength = std::max(digitCount(entry.ref), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxTabNrmWidth() const {
    return 3;
}
int SymbolTable::maxTabLvlWidth() const {
    int maxLength = 3;
    for (auto &&entry : tab_) {
        maxLength = std::max(digitCount(entry.lexicalLevel), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxTabAdrWidth() const {
    int maxLength = 3;
    for (auto &&entry : tab_) {
        maxLength = std::max(digitCount(entry.address), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxTabLinkWidth() const {
    int maxLength = 4;
    for (auto &&entry : tab_) {
        maxLength = std::max(digitCount(entry.link), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxTabValueWidth() const {
    int maxLength = 5;
    for (auto &&entry : tab_) {
        maxLength = std::max((int)entry.value.size(), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxTabInitWidth() const {
    return 12;
}

int SymbolTable::maxATabIdxWidth() const {
    return std::max(3, digitCount(atab_.size()));
}
int SymbolTable::maxATabIdxTypeWidth() const {
    return 11;
}
int SymbolTable::maxATabIdxRefWidth() const {
    int maxLength = 6;
    for (auto &&entry : atab_) {
        maxLength = std::max(digitCount(entry.indexRef), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxATabElTypeWidth() const {
    return 11;
}
int SymbolTable::maxATabElRefWidth() const {
    int maxLength = 6;
    for (auto &&entry : atab_) {
        maxLength = std::max(digitCount(entry.elementRef), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxATabLowWidth() const {
    int maxLength = 4;
    for (auto &&entry : atab_) {
        maxLength = std::max((int)entry.low.size(), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxATabHighWidth() const {
    int maxLength = 4;
    for (auto &&entry : atab_) {
        maxLength = std::max((int)entry.high.size(), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxATabResolvedWidth() const {
    return 8;
}
int SymbolTable::maxATabLowOrdWidth() const {
    int maxLength = 7;
    for (auto &&entry : atab_) {
        maxLength = std::max(digitCount(entry.lowOrdinal), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxATabHighOrdWidth() const {
    int maxLength = 7;
    for (auto &&entry : atab_) {
        maxLength = std::max(digitCount(entry.highOrdinal), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxATabElSizeWidth() const {
    int maxLength = 6;
    for (auto &&entry : atab_) {
        maxLength = std::max(digitCount(entry.elementSize), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxATabSizeWidth() const {
    int maxLength = 4;
    for (auto &&entry : atab_) {
        maxLength = std::max(digitCount(entry.size), maxLength);
    }
    return maxLength;
}

int SymbolTable::maxBTabIdxWidth() const {
    return std::max(3, digitCount(btab_.size()));
}
int SymbolTable::maxBTabNameWidth() const {
    int maxLength = 4;
    for (auto &&entry : btab_) {
        maxLength = std::max((int)entry.name.size(), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxBTabKindWidth() const {
    return 11;
}
int SymbolTable::maxBTabParentWidth() const {
    int maxLength = 6;
    for (auto &&entry : btab_) {
        if (entry.parent != -1) {
            maxLength = std::max(digitCount(entry.parent), maxLength);
        }
    }
    return maxLength;
}
int SymbolTable::maxBTabLastWidth() const {
    int maxLength = 4;
    for (auto &&entry : btab_) {
        maxLength = std::max(digitCount(entry.last), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxBTabLParWidth() const {
    int maxLength = 4;
    for (auto &&entry : btab_) {
        maxLength = std::max(digitCount(entry.lastParameter), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxBTabPSizeWidth() const {
    int maxLength = 5;
    for (auto &&entry : btab_) {
        maxLength = std::max(digitCount(entry.parameterSize), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxBTabVSizeWidth() const {
    int maxLength = 5;
    for (auto &&entry : btab_) {
        maxLength = std::max(digitCount(entry.variableSize), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxBTabLvlWidth() const {
    int maxLength = 3;
    for (auto &&entry : btab_) {
        maxLength = std::max(digitCount(entry.lexicalLevel), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxBTabReturnTypeWidth() const {
    return 11;
}
int SymbolTable::maxBTabReturnRefWidth() const {
    int maxLength = 9;
    for (auto &&entry : btab_) {
        maxLength = std::max(digitCount(entry.returnRef), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxBTabOwnerWidth() const {
    int maxLength = 5;
    for (auto &&entry : btab_) {
        maxLength = std::max(digitCount(entry.ownerTabIndex), maxLength);
    }
    return maxLength;
}

int SymbolTable::maxTypeDescriptorIdxWidth() const {
    return std::max(3, digitCount(typeDescriptors_.size()));
}
int SymbolTable::maxTypeDescriptorKindWidth() const {
    return 11;
}
int SymbolTable::maxTypeDescriptorBaseTypeWidth() const {
    return 11;
}
int SymbolTable::maxTypeDescriptorBaseRefWidth() const {
    int maxLength = 7;
    for (auto &&entry : typeDescriptors_) {
        maxLength = std::max(digitCount(entry.baseRef), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxTypeDescriptorLowWidth() const {
    int maxLength = 4;
    for (auto &&entry : typeDescriptors_) {
        maxLength = std::max((int)entry.low.size(), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxTypeDescriptorHighWidth() const {
    int maxLength = 4;
    for (auto &&entry : typeDescriptors_) {
        maxLength = std::max((int)entry.high.size(), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxTypeDescriptorResolvedWidth() const {
    return 8;
}
int SymbolTable::maxTypeDescriptorLowOrdWidth() const {
    int maxLength = 7;
    for (auto &&entry : typeDescriptors_) {
        maxLength = std::max(digitCount(entry.lowOrdinal), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxTypeDescriptorHighOrdWidth() const {
     int maxLength = 7;
    for (auto &&entry : typeDescriptors_) {
        maxLength = std::max(digitCount(entry.highOrdinal), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxTypeDescriptorSizeWidth() const {
    int maxLength = 4;
    for (auto &&entry : typeDescriptors_) {
        maxLength = std::max(digitCount(entry.size), maxLength);
    }
    return maxLength;
}
int SymbolTable::maxTypeDescriptorValuesWidth() const {
    int maxLength = 6;
    for (auto &&entry : typeDescriptors_) {
        maxLength = std::max((int) descriptorValuesToString(entry).size(), maxLength);
    }
    return maxLength;
}
