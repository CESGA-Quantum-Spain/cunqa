#pragma once

// Secure cast of size
template<typename TO, typename FROM>
TO legacy_size_cast(FROM value)
{
    static_assert(std::is_unsigned_v<FROM> && std::is_unsigned_v<TO>,
                  "Only unsigned types can be cast here!");
    TO result = value;
    return result;
}