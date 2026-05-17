#ifndef ARION_SYMBOL_TABLE_H
#define ARION_SYMBOL_TABLE_H

#include <stdexcept>
#include <string>
#include <vector>

namespace arion {
    enum class SymbolObjectKind {
        Reserved,
        Program,
        Constant,
        Type,
        Variable,
        Procedure,
        Function,
        Parameter,
        Field,
        Unknown
    };

    enum class TypeKind {
        Unknown,
        Void,
        Integer,
        Real,
        Boolean,
        Char,
        String,
        Subrange,
        Array,
        Record,
        Enumerated
    };

    enum class BlockKind {
        Global,
        Program,
        Procedure,
        Function,
        Record,
        Anonymous
    };

    enum class TypeDescriptorKind {
        Unknown,
        Subrange,
        Enumerated
    };

    struct TabEntry {
        std::string identifier;
        int link = 0;
        SymbolObjectKind object = SymbolObjectKind::Unknown;
        TypeKind type = TypeKind::Unknown;
        int ref = 0;
        bool normal = true;
        int lexicalLevel = 0;
        int address = 0;
        std::string value;
        bool initialized = false;
    };

    struct ATabEntry {
        TypeKind indexType = TypeKind::Unknown;
        TypeKind elementType = TypeKind::Unknown;
        int elementRef = 0;
        std::string low;
        std::string high;
        int elementSize = 1;
        int size = 0;
        int indexRef = 0;
        bool boundsResolved = false;
        int lowOrdinal = 0;
        int highOrdinal = 0;
    };

    struct BTabEntry {
        std::string name;
        int parent = -1;
        int last = 0;
        int lastParameter = 0;
        int parameterSize = 0;
        int variableSize = 0;
        int lexicalLevel = 0;
        BlockKind kind = BlockKind::Anonymous;
        TypeKind returnType = TypeKind::Void;
        int returnRef = 0;
        int ownerTabIndex = -1;
    };

    struct TypeDescriptor {
        TypeDescriptorKind kind = TypeDescriptorKind::Unknown;
        TypeKind baseType = TypeKind::Unknown;
        int baseRef = 0;
        std::string low;
        std::string high;
        bool boundsResolved = false;
        int lowOrdinal = 0;
        int highOrdinal = 0;
        std::vector<std::string> values;
        int size = 1;
    };

    class SymbolTable {
    public:
        SymbolTable();

        void reset();

        int enterBlock(const std::string &name = "");
        int enterBlock(const std::string &name, BlockKind kind);
        void enterBlockByIndex(int blockIndex);
        void leaveBlock();
        int currentBlockIndex() const;
        int currentLexicalLevel() const;

        int declare(TabEntry entry);
        int declareProgram(const std::string &name);
        int declareConstant(const std::string &name, TypeKind type, std::string value);
        int declareType(const std::string &name, TypeKind type, int ref = 0);
        int declareVariable(const std::string &name, TypeKind type, int ref = 0);
        int declareParameter(const std::string &name, TypeKind type, int ref = 0, bool normal = true);
        int declareField(const std::string &name, TypeKind type, int ref = 0);
        int declareProcedure(const std::string &name, int blockRef = 0);
        int declareFunction(const std::string &name, TypeKind returnType, int blockRef = 0);

        int addArray(ATabEntry entry);
        int addArrayType(TypeKind indexType, int indexRef, const std::string &low,
                         const std::string &high, TypeKind elementType, int elementRef = 0);
        int declareArrayType(const std::string &name, TypeKind indexType, int indexRef,
                             const std::string &low, const std::string &high,
                             TypeKind elementType, int elementRef = 0);

        int addBlock(BTabEntry entry);
        int createBlock(const std::string &name, BlockKind kind, int parentBlock = -2);
        int createRecordBlock(const std::string &name = "");
        int beginRecordType(const std::string &name = "");
        void endRecordType();
        int declareRecordType(const std::string &name, int recordBlockRef);
        int createProcedureBlock(const std::string &name = "");
        int createFunctionBlock(const std::string &name = "");
        int declareProcedureWithBlock(const std::string &name);
        int declareFunctionWithBlock(const std::string &name, TypeKind returnType, int returnRef = 0);

        int addSubrange(TypeKind baseType, const std::string &low, const std::string &high, int baseRef = 0);
        int declareSubrangeType(const std::string &name, TypeKind baseType,
                                const std::string &low, const std::string &high, int baseRef = 0);
        int addEnumeratedType(const std::vector<std::string> &values);
        int declareEnumeratedType(const std::string &name, const std::vector<std::string> &values);
        int declareEnumeratedConstant(const std::string &name, int enumeratedRef, int ordinal);

        int lookupIndex(const std::string &identifier) const;
        int lookupCurrentScopeIndex(const std::string &identifier) const;
        const TabEntry *lookup(const std::string &identifier) const;
        const TabEntry *lookupCurrentScope(const std::string &identifier) const;
        int requireLookupIndex(const std::string &identifier) const;
        const TabEntry &requireLookup(const std::string &identifier) const;
        int requireTypeIndex(const std::string &identifier) const;
        const TabEntry &requireType(const std::string &identifier) const;
        const ATabEntry &requireArray(int ref) const;
        const BTabEntry &requireBlock(int ref) const;
        const TypeDescriptor &requireTypeDescriptor(int ref) const;

        int typeSize(TypeKind type, int ref = 0) const;

        const std::vector<TabEntry> &tab() const;
        const std::vector<BTabEntry> &btab() const;
        const std::vector<ATabEntry> &atab() const;
        const std::vector<TypeDescriptor> &typeDescriptors() const;

        std::string dumpTab() const;
        std::string dumpBTab() const;
        std::string dumpATab() const;
        std::string dumpTypeDescriptors() const;

        static std::string objectKindToString(SymbolObjectKind kind);
        static std::string typeKindToString(TypeKind kind);
        static std::string blockKindToString(BlockKind kind);
        static std::string descriptorKindToString(TypeDescriptorKind kind);

    private:
        std::vector<TabEntry> tab_;
        std::vector<BTabEntry> btab_;
        std::vector<ATabEntry> atab_;
        std::vector<TypeDescriptor> typeDescriptors_;
        std::vector<int> blockStack_;
        int predefinedLimit_ = 0;

        void initializePredefinedIdentifiers();
        int appendEntry(TabEntry entry, bool updateCurrentBlock);
        int lookupPredefinedIndex(const std::string &identifier) const;
        int currentBlockLast() const;
        void setCurrentBlockLast(int tabIndex);
        std::string normalize(const std::string &identifier) const;
        bool sameIdentifier(const std::string &left, const std::string &right) const;

        bool isValidTabIndex(int index) const;
        bool isValidArrayRef(int ref) const;
        bool isValidBlockRef(int ref) const;
        bool isValidTypeDescriptorRef(int ref) const;
        void ensureDeclarableIdentifier(const std::string &identifier) const;
        void validateSubrangeBase(TypeKind baseType) const;
        void validateArrayIndexType(TypeKind indexType) const;
        bool parseIntegerLiteral(const std::string &text, int &value) const;
        bool ordinalFromLiteral(TypeKind type, int ref, const std::string &text, int &ordinal) const;
        bool resolveBounds(TypeKind type, int ref, const std::string &low,
                           const std::string &high, int &lowOrdinal, int &highOrdinal) const;
        int storageSize(TypeKind type, int ref) const;
        std::string descriptorValuesToString(const TypeDescriptor &descriptor) const;
    };

    class SymbolTableError : public std::runtime_error {
    public:
        explicit SymbolTableError(const std::string &message);
    };
}

#endif
