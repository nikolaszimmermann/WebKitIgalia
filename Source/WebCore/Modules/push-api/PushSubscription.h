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

#pragma once

#if ENABLE(SERVICE_WORKER)

#include "EpochTimeStamp.h"
#include "ExceptionOr.h"
#include "JSDOMPromiseDeferred.h"
#include "PushEncryptionKeyName.h"
#include "PushSubscriptionJSON.h"

#include <optional>
#include <variant>
#include <wtf/IsoMalloc.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class PushSubscriptionOptions;

class PushSubscription : public RefCounted<PushSubscription> {
    WTF_MAKE_ISO_ALLOCATED_EXPORT(PushSubscription, WEBCORE_EXPORT);
public:
    template<typename... Args> static Ref<PushSubscription> create(Args&&... args) { return adoptRef(*new PushSubscription(std::forward<Args>(args)...)); }
    WEBCORE_EXPORT ~PushSubscription();

    const String& endpoint() const;
    std::optional<EpochTimeStamp> expirationTime() const;
    PushSubscriptionOptions& options() const;
    ExceptionOr<RefPtr<JSC::ArrayBuffer>> getKey(PushEncryptionKeyName) const;
    void unsubscribe(DOMPromiseDeferred<IDLBoolean>&&);

    PushSubscriptionJSON toJSON() const;

private:
    WEBCORE_EXPORT PushSubscription(String&& endpoint, std::optional<EpochTimeStamp> expirationTime, Ref<PushSubscriptionOptions>&&, Vector<uint8_t>&& clientECDHPublicKey, Vector<uint8_t>&& auth);

    String m_endpoint;
    std::optional<EpochTimeStamp> m_expirationTime;
    Ref<PushSubscriptionOptions> m_options;
    Vector<uint8_t> m_clientECDHPublicKey;
    Vector<uint8_t> m_sharedAuthenticationSecret;
};

} // namespace WebCore

#endif // ENABLE(SERVICE_WORKER)
