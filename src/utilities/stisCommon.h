template<typename T>
void WriteProtected(uintptr_t address, T value) {
    DWORD oldProtect;
    VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect);
    *reinterpret_cast<T*>(address) = value;
    VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), oldProtect, &oldProtect);
}

inline csl::math::Vector3 ConvertTo3(const csl::math::Vector4& vector4) {
    return vector4.template head<3>();
}

template<typename Param, typename dataType>
inline void SetAllModes(app::player::PlayerParameters* params, Param app::player::ModePackage::* package, dataType Param::* field, const dataType& value) {
    params->forwardView.*package.*field = value;
    params->whiteSpace.*package.*field = value;
    params->sideView.*package.*field = value;
    params->boss.*package.*field = value;
}