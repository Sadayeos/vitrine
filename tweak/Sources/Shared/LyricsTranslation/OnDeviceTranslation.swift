// The two ways to translate a song on the iPhone itself, for LyricsTranslation.h: Apple's Translation
// framework (iOS 26, with the languages downloaded in the Translate app) and Apple Intelligence's language
// model (iOS 26, on the iPhones that have it). Both are Swift only. Nothing leaves the phone.
import Foundation
import NaturalLanguage
import os

@objc(SGOnDeviceTranslation)
public final class SGOnDeviceTranslation: NSObject {
    private static let log = Logger(subsystem: "spotifyglass", category: "translation")

    private static func finish(_ done: @escaping ([String]?, String?) -> Void, _ lines: [String]?, _ error: String?) {
        DispatchQueue.main.async { done(lines, error) }
    }

    // MARK: - Translation Framework

    @objc public static var translationAvailable: Bool {
        return false
    }

    @objc public static func translate(_ lines: [String], to languageTag: String, done: @escaping ([String]?, String?) -> Void) {
        finish(done, nil, "Apple's Translate API is not supported on this CI build environment.")
    }

    // MARK: - Apple Intelligence

    @objc public static func appleIntelligenceAvailable(_ languageTag: String) -> Bool {
        return false
    }

    @objc public static func translateWithAppleIntelligence(_ lines: [String], to languageTag: String, song: String?, progress: @escaping ([String]) -> Void,
                                                            done: @escaping ([String]?, String?) -> Void) {
        finish(done, nil, "Apple Intelligence is not available on this CI build environment.")
    }
}