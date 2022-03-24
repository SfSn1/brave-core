import { renderHook, act } from '@testing-library/react-hooks'

import * as WalletActions from '../actions/wallet_actions'
import useAssetManagement from './assets-management'
import { AccountAssetOptions } from '../../options/asset-options'

const mockUserVisibleTokensInfo = [
  AccountAssetOptions[1],
  AccountAssetOptions[2]
]

const mockCustomToken = {
  contractAddress: 'customTokenContractAddress',
  name: 'Custom Token',
  symbol: 'CT',
  logo: '',
  isErc20: true,
  isErc721: false,
  decimals: 18,
  visible: true,
  tokenId: '',
  coingeckoId: '',
  chainId: '0x1'
}

describe('useAssetManagement hook', () => {
  let addedAssetSpy: jest.SpyInstance
  let removedAssetSpy: jest.SpyInstance
  let refreshedBalancesPricesSpy: jest.SpyInstance
  let setUserAssetVisibleSpy: jest.SpyInstance

  addedAssetSpy = jest.spyOn(WalletActions, 'addUserAsset')
  removedAssetSpy = jest.spyOn(WalletActions, 'removeUserAsset')
  refreshedBalancesPricesSpy = jest.spyOn(WalletActions, 'refreshBalancesAndPriceHistory')
  setUserAssetVisibleSpy = jest.spyOn(WalletActions, 'setUserAssetVisible')

  it('Should add an asset', () => {
    const { result } = renderHook(() => useAssetManagement(
      WalletActions.addUserAsset,
      WalletActions.setUserAssetVisible,
      WalletActions.removeUserAsset,
      WalletActions.refreshBalancesAndPriceHistory,
      AccountAssetOptions,
      mockUserVisibleTokensInfo
    ))
    act(() => result.current.onUpdateVisibleAssets([AccountAssetOptions[1], AccountAssetOptions[2], AccountAssetOptions[3]]))
    expect(addedAssetSpy).toBeCalledWith({ ...AccountAssetOptions[3], logo: '' })
    expect(refreshedBalancesPricesSpy).toBeCalledTimes(1)
  })

  it('Should remove an asset', () => {
    const { result } = renderHook(() => useAssetManagement(
      WalletActions.addUserAsset,
      WalletActions.setUserAssetVisible,
      WalletActions.removeUserAsset,
      WalletActions.refreshBalancesAndPriceHistory,
      AccountAssetOptions,
      mockUserVisibleTokensInfo
    ))
    act(() => result.current.onUpdateVisibleAssets([mockUserVisibleTokensInfo[0]]))
    expect(removedAssetSpy).toBeCalledWith(mockUserVisibleTokensInfo[1])
    expect(refreshedBalancesPricesSpy).toBeCalledTimes(1)
  })

  it('Should remove and add an asset', () => {
    const { result } = renderHook(() => useAssetManagement(
      WalletActions.addUserAsset,
      WalletActions.setUserAssetVisible,
      WalletActions.removeUserAsset,
      WalletActions.refreshBalancesAndPriceHistory,
      AccountAssetOptions,
      mockUserVisibleTokensInfo
    ))
    act(() => result.current.onUpdateVisibleAssets([AccountAssetOptions[1], AccountAssetOptions[3]]))
    expect(addedAssetSpy).toBeCalledWith({ ...AccountAssetOptions[3], logo: '' })
    expect(removedAssetSpy).toBeCalledWith(AccountAssetOptions[2])
    expect(refreshedBalancesPricesSpy).toBeCalledTimes(1)
  })

  it('Should set custom tokens visibility to false', () => {
    const { result } = renderHook(() => useAssetManagement(
      WalletActions.addUserAsset,
      WalletActions.setUserAssetVisible,
      WalletActions.removeUserAsset,
      WalletActions.refreshBalancesAndPriceHistory,
      AccountAssetOptions,
      [...mockUserVisibleTokensInfo, mockCustomToken]
    ))
    act(() => result.current.onUpdateVisibleAssets([...mockUserVisibleTokensInfo, { ...mockCustomToken, visible: false }]))
    expect(setUserAssetVisibleSpy).toBeCalledWith({ token: { ...mockCustomToken, visible: false }, isVisible: false })
    expect(refreshedBalancesPricesSpy).toBeCalledTimes(1)
  })

  it('Should set custom tokens visibility to true', () => {
    const { result } = renderHook(() => useAssetManagement(
      WalletActions.addUserAsset,
      WalletActions.setUserAssetVisible,
      WalletActions.removeUserAsset,
      WalletActions.refreshBalancesAndPriceHistory,
      AccountAssetOptions,
      [...mockUserVisibleTokensInfo, { ...mockCustomToken, visible: false }]
    ))
    act(() => result.current.onUpdateVisibleAssets([...mockUserVisibleTokensInfo, mockCustomToken]))
    expect(setUserAssetVisibleSpy).toBeCalledWith({ token: mockCustomToken, isVisible: true })
    expect(refreshedBalancesPricesSpy).toBeCalledTimes(1)
  })
})
