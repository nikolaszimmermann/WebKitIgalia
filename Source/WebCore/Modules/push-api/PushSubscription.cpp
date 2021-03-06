/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PushSubscription.h"

#if ENABLE(SERVICE_WORKER)

#include "Exception.h"
#include "PushSubscriptionOptions.h"
#include <wtf/IsoMallocInlines.h>
#include <wtf/text/Base64.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(PushSubscription);

PushSubscription::PushSubscription(String&& endpoint, std::optional<EpochTimeStamp> expirationTime, Ref<PushSubscriptionOptions>&& options, Vector<uint8_t>&& clientECDHPublicKey, Vector<uint8_t>&& sharedAuthenticationSecret)
    : m_endpoint(WTFMove(endpoint))
    , m_expirationTime(expirationTime)
    , m_options(WTFMove(options))
    , m_clientECDHPublicKey(WTFMove(clientECDHPublicKey))
    , m_sharedAuthenticationSecret(WTFMove(sharedAuthenticationSecret))
{
}

PushSubscription::~PushSubscription() = default;

const String& PushSubscription::endpoint() const
{
    return m_endpoint;
}

std::optional<EpochTimeStamp> PushSubscription::expirationTime() const
{
    return m_expirationTime;
}

PushSubscriptionOptions& PushSubscription::options() const
{
    return m_options;
}

ExceptionOr<RefPtr<JSC::ArrayBuffer>> PushSubscription::getKey(PushEncryptionKeyName name) const
{
    const Vector<uint8_t> *source = nullptr;

    switch (name) {
    case PushEncryptionKeyName::P256dh:
        source = &m_clientECDHPublicKey;
        break;
    case PushEncryptionKeyName::Auth:
        source = &m_sharedAuthenticationSecret;
        break;
    default:
        return nullptr;
    }

    auto buffer = ArrayBuffer::tryCreate(source->data(), source->size());
    if (!buffer)
        return Exception { OutOfMemoryError };
    return buffer;
}

void PushSubscription::unsubscribe(DOMPromiseDeferred<IDLBoolean>&& promise)
{
    promise.reject(Exception { NotSupportedError, "Not implemented"_s });
}

PushSubscriptionJSON PushSubscription::toJSON() const
{
    return PushSubscriptionJSON {
        m_endpoint,
        m_expirationTime,
        Vector<WTF::KeyValuePair<String, String>> {
            { "p256dh"_s, base64URLEncodeToString(m_clientECDHPublicKey) },
            { "auth"_s, base64URLEncodeToString(m_sharedAuthenticationSecret) }
        }
    };
}

} // namespace WebCore

#endif // ENABLE(SERVICE_WORKER)
