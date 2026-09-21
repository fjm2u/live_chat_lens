# Live Chat Lens

Live Chat Lens is a third-party native dock for OBS Studio. Its on-screen name is **AI Comments**. It reads new messages from a YouTube live chat, asks the Jev API to classify them, and shows up to ten recommendations to the streamer. It does not post messages, moderate YouTube chat, or add an overlay to the stream. It is not affiliated with the OBS Project.

**Status:** source preview. There is no supported binary release yet. The macOS arm64 module has been built, loaded in OBS Studio 32.2.2, and its dock has been inspected locally. End-to-end testing with a real YouTube live chat, sustained streaming tests, distribution signing and notarization, and Windows/Linux testing remain open. See [release status](docs/release-status.md).

## How it works

1. Enter a live video URL or ID, a YouTube Data API key, and a Jev API key, then select **Docks → AI Comments** in OBS Studio. The first fetched chat page establishes a cursor; older messages are not recommended.
2. The plugin polls the official YouTube API, filters empty, oversized, duplicate, URL-only, and rapid repeat messages, and sends batches of new comment text to Jev for classification.
3. The dock ranks the classified messages and displays at most ten. A star marks high-priority messages. 👍 records a useful recommendation; × removes a card from the current view.

Network requests, JSON processing, and event logging run in a worker `QThread`; the OBS UI thread renders the dock. The [Japanese README](README.md#アーキテクチャ) contains a component diagram and implementation details.

## Requirements and development build

- OBS Studio 30 or newer with the frontend dock API, Qt 6.6 or newer, and the matching OBS development SDK.
- A YouTube Data API key for reading a public live chat and a Jev API key. API usage and quota are the user's responsibility.
- The current local build instructions and core-only test commands are in the [Japanese README](README.md#ビルド). The macOS fallback build also needs official OBS source headers, which are intentionally not committed to this repository.

No installer or prebuilt download is available. Do not distribute the local `build/obs-comment-dock.plugin` bundle as a release: it is an ad-hoc-signed development build and has not been notarized or validated on another machine.

## Data and credentials

Comment text is sent to the Jev API for classification. The plugin reads live-chat data from the YouTube Data API. It stores the video URL and display preferences in OBS plugin settings, and writes event metadata to `events.jsonl` in the OBS plugin configuration directory. The event log includes comment IDs, labels, timestamps, and latency, but not comment text or author names. API keys are held in memory and are not written to the settings or event log. Avoid sharing OBS logs, settings, or screenshots before checking them for sensitive data.

The published version accepts API keys. Any other locally developed authentication flow is outside the current published release until committed and verified.

## Support and project status

Use [GitHub Issues](https://github.com/fjm2u/live_chat_lens/issues) for bugs and compatibility reports. Include the OBS version, operating system and architecture, reproduction steps, and redacted log excerpts. Do not post API keys, OAuth files, refresh tokens, or live-chat content that you do not have permission to share.

The test and acceptance boundaries are recorded in [verification](docs/verification.md), [acceptance criteria](docs/acceptance.md), and [release status](docs/release-status.md). Source availability does not mean that the plugin is ready for an OBS forum resource listing.

## License and development disclosure

The source is offered under **GPL-2.0-or-later**; see [LICENSE](LICENSE). This matches the GPL v2-or-later terms used by OBS Studio and libobs, which this plugin links against. Binary distributors must also satisfy the terms of the Qt components they ship.

Codex AI assistance was used in developing the code and documentation. The plugin still requires independent review and live-use validation before a stable release. The [OBS resource policy](https://obsproject.com/forum/threads/forum-resource-and-ip-policy.178569/) requires disclosure of AI use and says resources written entirely or largely with AI coding tools are not permitted. Disclosure alone does not guarantee that this project is eligible for an OBS forum listing.
