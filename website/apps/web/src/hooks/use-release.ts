import type { Endpoints } from '@octokit/types'

import useSWR from 'swr'

export type ReleaseAsset =
  Endpoints['GET /repos/{owner}/{repo}/releases/latest']['response']['data']['assets'][number] & {
    mirrors: string[]
  }

export type Release = Omit<
  Endpoints['GET /repos/{owner}/{repo}/releases/latest']['response']['data'],
  'assets'
> & { assets: ReleaseAsset[] }

export const useRelease = () => {
  const { data, ...rest } = useSWR<{
    details: Release
  }>('https://ota.maa.plus/MaaAssistantArknights/api/version/stable.json')
  return {
    data: data?.details,
    ...rest,
  }
}
