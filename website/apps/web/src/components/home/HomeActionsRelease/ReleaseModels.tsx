import { Release, ReleaseAsset } from '@/hooks/use-release'
import mdiApple from '@iconify/icons-mdi/apple'
import mdiLinux from '@iconify/icons-mdi/linux'
import mdiWindows from '@iconify/icons-mdi/windows'
import type { IconifyIcon } from '@iconify/react'

export interface PlatformPredicate {
  id: string
  icon: IconifyIcon
  title: string
  subtitle: string
  messages: {
    downloaded: string
  }
  assetMatcher: (release: Release) => ReleaseAsset | undefined
}

export const PLATFORMS: PlatformPredicate[] = [
  {
    id: 'windows-x64',
    icon: mdiWindows,
    title: 'platforms.windows-x64.title',
    subtitle: 'platforms.windows-x64.subtitle',
    messages: {
      downloaded: 'platforms.windows-x64.messages.downloaded',
    },
    assetMatcher: (release) => {
      return release.assets.find((el) => /^MAA-v.*-win-x64\.zip/.test(el.name))
    },
  },
  {
    id: 'windows-arm64',
    icon: mdiWindows,
    title: 'platforms.windows-arm64.title',
    subtitle: 'platforms.windows-arm64.subtitle',
    messages: {
      downloaded: 'platforms.windows-arm64.messages.downloaded',
    },
    assetMatcher: (release) => {
      return release.assets.find((el) =>
        /^MAA-v.*-win-arm64\.zip/.test(el.name),
      )
    },
  },
  {
    id: 'macos-universal',
    icon: mdiApple,
    title: 'platforms.macos-universal.title',
    subtitle: 'platforms.macos-universal.subtitle',
    messages: {
      downloaded: 'platforms.macos-universal.messages.downloaded',
    },
    assetMatcher: (release) => {
      return release.assets.find((el) =>
        /^MAA-v.*-macos-universal\.dmg/.test(el.name),
      )
    },
  },
  {
    id: 'linux-x64',
    icon: mdiLinux,
    title: 'platforms.linux-x64.title',
    subtitle: 'platforms.linux-x64.subtitle',
    messages: {
      downloaded: 'platforms.linux-x64.messages.downloaded',
    },
    assetMatcher: (release) => {
      return release.assets.find((el) =>
        /^MAA-v.*-linux-x86_64\.tar\.gz/.test(el.name),
      )
    },
  },
  {
    id: 'linux-aarch64',
    icon: mdiLinux,
    title: 'platforms.linux-aarch64.title',
    subtitle: 'platforms.linux-aarch64.subtitle',
    messages: {
      downloaded: 'platforms.linux-aarch64.messages.downloaded',
    },
    assetMatcher: (release) => {
      return release.assets.find((el) =>
        /^MAA-v.*-linux-aarch64\.tar\.gz/.test(el.name),
      )
    },
  },
]

// detectPlatform detects the platform of the current user and returns the
// corresponding platform ID. The detector should be as accurate as possible,
// and it should take account the user's architecture and OS.
// The more modern navigator.userAgentData should be used if available.
export const DetectionFailedSymbol = Symbol('detectionFailed')
export const detectPlatform = async (): Promise<
  string | typeof DetectionFailedSymbol
> => {
  if (typeof navigator === 'undefined') {
    return DetectionFailedSymbol
  }

  const userAgentData = await (
    navigator as any
  ).userAgentData?.getHighEntropyValues(['platform', 'architecture'])

  if (userAgentData) {
    const { platform, architecture } = userAgentData

    if (platform === 'macOS') {
      return 'macos-universal'
    }

    if (platform === 'Windows') {
      if (architecture.startsWith('arm')) {
        return 'windows-arm64'
      }
      return 'windows-x64'
    }

    if (platform === 'Linux') {
      if (architecture.startsWith('arm')) {
        return 'linux-aarch64'
      }
      return 'linux-x64'
    }
  }

  const { userAgent } = navigator

  const lowerCaseUA = userAgent.toLowerCase()

  if (lowerCaseUA.includes('windows')) {
    if (lowerCaseUA.includes('arm')) {
      return 'windows-arm64'
    }
    return 'windows-x64'
  }

  if (lowerCaseUA.includes('macintosh')) {
    return 'macos-universal'
  }

  if (lowerCaseUA.includes('linux')) {
    if (lowerCaseUA.includes('aarch64') || lowerCaseUA.includes('arm64')) {
      return 'linux-aarch64'
    }
    return 'linux-x64'
  }

  return DetectionFailedSymbol
}

export interface ResolvedPlatform {
  asset: ReleaseAsset
  platform: PlatformPredicate
}
