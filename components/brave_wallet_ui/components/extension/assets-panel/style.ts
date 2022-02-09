import styled from 'styled-components'
import { WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  margin-bottom: 80px;
`

export const AddAssetButton = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 0px;
  margin: 4px 0px;
  font-family: Poppins;
  font-size: 14px;
  letter-spacing: 0.01em;
  line-height: 20px;
  color: ${(p) => p.theme.palette.blurple500};
  @media (prefers-color-scheme: dark) {
    color: ${(p) => p.theme.color.interactive06};
  }
`
