import * as React from 'react'
import { OriginInfo } from '../../../constants/types'

export interface Props {
  originInfo: OriginInfo
}

const CreateSiteOrigin = (props: Props) => {
  const { originInfo } = props

  const url = React.useMemo(() => {
    if (originInfo.eTldPlusOne) {
      const before = originInfo.origin.split(originInfo.eTldPlusOne)[0]
      const after = originInfo.origin.split(originInfo.eTldPlusOne)[1]
      // Will inherit styling from parent container
      return <span>{before}<b>{originInfo.eTldPlusOne}</b>{after}</span>
    }
    return <span>{originInfo}</span>
  }, [originInfo])

  return (<>{url}</>)
}
export default CreateSiteOrigin
