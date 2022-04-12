#pragma once

#include <atlbase.h>
#include <optional>
#include <functional>
#include <string>
#include <vector>
#include <PortableDeviceApi.h>

namespace mtp
{

    template<typename T> concept HasArrowOperator = requires(T v) { v.operator->(); };

    template<typename T> class ExpectedOrHResult
    {
        HRESULT hr{ S_OK };
        T v{};

    public:
        template<typename U> ExpectedOrHResult(U&& value) : v(std::move(value)) { }
        ExpectedOrHResult(HRESULT hr) : hr(hr) { }

        operator bool() const { return SUCCEEDED(hr); }
        auto GetResult() const { return hr; }
        auto& GetValue() const { return v; }

        auto& operator*() { return v; }

        auto operator->() requires (HasArrowOperator<T>) {
            return v.operator->();
        }

        T* operator->() requires (!HasArrowOperator<T>) {
            return &v;
        }
    };

    using DeviceID = std::string;
    using ObjectID = std::string;

    struct PortableDevice
    {
        DeviceID id;
        std::optional<std::string> friendlyName;
        std::optional<std::string> manufacturer;
        std::optional<std::string> description;
    };

    struct ObjectProperties
    {
        std::optional<std::string> name;
        std::optional<std::string> fileName;
        std::optional<GUID> contentType;
        std::optional<GUID> format;
        std::optional<ULONG> size;
    };

    ExpectedOrHResult<std::vector<PortableDevice>> EnumeratePortableDevices();
    ExpectedOrHResult<CComPtr<IPortableDevice>> OpenDevice(const DeviceID&);

    ExpectedOrHResult<ObjectProperties> ReadProperties(CComPtr<IPortableDevice>& device, const ObjectID&);
    ExpectedOrHResult<std::vector<ObjectID>> EnumerateContents(CComPtr<IPortableDevice>& device, const ObjectID&);

    ExpectedOrHResult<ObjectID> Lookup(CComPtr<IPortableDevice>& device, const std::vector<std::string>& path);

    using ReadCallbackFn = std::function<bool(const void*, size_t)>;
    ExpectedOrHResult<size_t> ReadData(CComPtr<IPortableDevice>& device, const ObjectID&, ReadCallbackFn callback);
}
