#include "SymbolTable.hpp"
#include <cctype>
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

    tab_.push_back(TabEntry{"<nil>", 0, SymbolObjectKind::Reserved, TypeKind::Void, 0, true, 0, 0, "", true});
    initializePredefinedIdentifiers();
    predefinedLimit_ = static_cast<int>(tab_.size());

    BTabEntry global;
    global.name = "<global>";
    global.parent = -1;
    global.lexicalLevel = 0;
    global.kind = BlockKind::Global;
    btab_.push_back(global);
    blockStack_.push_back(0);
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
    } else if (entry.size < 0) {
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
    ensureDeclarableIdentifier(name);
    for (const std::string &value : values) {
        if (sameIdentifier(name, value)) {
            throw SymbolTableError("Enumerated value conflicts with type name: " + value);
        }
        ensureDeclarableIdentifier(value);
    }

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
    out << "idx identifier object type ref nrm lev adr link value initialized\n";
    for (std::size_t i = 0; i < tab_.size(); ++i) {
        const TabEntry &entry = tab_[i];
        out << i << ' '
            << entry.identifier << ' '
            << objectKindToString(entry.object) << ' '
            << typeKindToString(entry.type) << ' '
            << entry.ref << ' '
            << (entry.normal ? 1 : 0) << ' '
            << entry.lexicalLevel << ' '
            << entry.address << ' '
            << entry.link << ' '
            << entry.value << ' '
            << (entry.initialized ? 1 : 0) << '\n';
    }
    return out.str();
}

std::string SymbolTable::dumpBTab() const {
    std::ostringstream out;
    out << "idx name kind parent last lpar psze vsze lev returnType returnRef owner\n";
    for (std::size_t i = 0; i < btab_.size(); ++i) {
        const BTabEntry &entry = btab_[i];
        out << i << ' '
            << entry.name << ' '
            << blockKindToString(entry.kind) << ' '
            << entry.parent << ' '
            << entry.last << ' '
            << entry.lastParameter << ' '
            << entry.parameterSize << ' '
            << entry.variableSize << ' '
            << entry.lexicalLevel << ' '
            << typeKindToString(entry.returnType) << ' '
            << entry.returnRef << ' '
            << entry.ownerTabIndex << '\n';
    }
    return out.str();
}

std::string SymbolTable::dumpATab() const {
    std::ostringstream out;
    out << "idx xtyp xref etyp eref low high resolved lowOrd highOrd elsz size\n";
    for (std::size_t i = 0; i < atab_.size(); ++i) {
        const ATabEntry &entry = atab_[i];
        out << i << ' '
            << typeKindToString(entry.indexType) << ' '
            << entry.indexRef << ' '
            << typeKindToString(entry.elementType) << ' '
            << entry.elementRef << ' '
            << entry.low << ' '
            << entry.high << ' '
            << (entry.boundsResolved ? 1 : 0) << ' '
            << entry.lowOrdinal << ' '
            << entry.highOrdinal << ' '
            << entry.elementSize << ' '
            << entry.size << '\n';
    }
    return out.str();
}

std::string SymbolTable::dumpTypeDescriptors() const {
    std::ostringstream out;
    out << "idx kind baseType baseRef low high resolved lowOrd highOrd size values\n";
    for (std::size_t i = 0; i < typeDescriptors_.size(); ++i) {
        const TypeDescriptor &entry = typeDescriptors_[i];
        out << i << ' '
            << descriptorKindToString(entry.kind) << ' '
            << typeKindToString(entry.baseType) << ' '
            << entry.baseRef << ' '
            << entry.low << ' '
            << entry.high << ' '
            << (entry.boundsResolved ? 1 : 0) << ' '
            << entry.lowOrdinal << ' '
            << entry.highOrdinal << ' '
            << entry.size << ' '
            << descriptorValuesToString(entry) << '\n';
    }
    return out.str();
}

std::string SymbolTable::objectKindToString(SymbolObjectKind kind) {
    switch (kind) {
        case SymbolObjectKind::Reserved: return "reserved";
        case SymbolObjectKind::Program: return "program";
        case SymbolObjectKind::Constant: return "constant";
        case SymbolObjectKind::Type: return "type";
        case SymbolObjectKind::Variable: return "variable";
        case SymbolObjectKind::Procedure: return "procedure";
        case SymbolObjectKind::Function: return "function";
        case SymbolObjectKind::Parameter: return "parameter";
        case SymbolObjectKind::Field: return "field";
        case SymbolObjectKind::Unknown: return "unknown";
    }
    return "unknown";
}

std::string SymbolTable::typeKindToString(TypeKind kind) {
    switch (kind) {
        case TypeKind::Unknown: return "unknown";
        case TypeKind::Void: return "void";
        case TypeKind::Integer: return "integer";
        case TypeKind::Real: return "real";
        case TypeKind::Boolean: return "boolean";
        case TypeKind::Char: return "char";
        case TypeKind::String: return "string";
        case TypeKind::Subrange: return "subrange";
        case TypeKind::Array: return "array";
        case TypeKind::Record: return "record";
        case TypeKind::Enumerated: return "enumerated";
    }
    return "unknown";
}

std::string SymbolTable::blockKindToString(BlockKind kind) {
    switch (kind) {
        case BlockKind::Global: return "global";
        case BlockKind::Program: return "program";
        case BlockKind::Procedure: return "procedure";
        case BlockKind::Function: return "function";
        case BlockKind::Record: return "record";
        case BlockKind::Anonymous: return "anonymous";
    }
    return "anonymous";
}

std::string SymbolTable::descriptorKindToString(TypeDescriptorKind kind) {
    switch (kind) {
        case TypeDescriptorKind::Unknown: return "unknown";
        case TypeDescriptorKind::Subrange: return "subrange";
        case TypeDescriptorKind::Enumerated: return "enumerated";
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
    appendEntry(TabEntry{"writeln", 0, SymbolObjectKind::Procedure, TypeKind::Void, 0, true, 0, 0, "", true}, false);
    appendEntry(TabEntry{"write", 0, SymbolObjectKind::Procedure, TypeKind::Void, 0, true, 0, 0, "", true}, false);
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
