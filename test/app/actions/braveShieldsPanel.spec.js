/* global describe, it */

import assert from 'assert'
import * as types from '../../../app/constants/shieldsPanelTypes'
import * as actions from '../../../app/actions/shieldsPanelActions'

describe('shieldsPanelActions', () => {
  it('shieldsPanelDataUpdated', () => {
    const details = {
      adBlock: 'allow',
      trackingProtection: 'block',
      origin: 'https://www.brave.com',
      hostname: 'www.brave.com'
    }
    assert.deepEqual(actions.shieldsPanelDataUpdated(details), {
      type: types.SHIELDS_PANEL_DATA_UPDATED,
      details
    })
  })

  it('shieldsToggled', () => {
    assert.deepEqual(actions.shieldsToggled(), {
      type: types.SHIELDS_TOGGLED
    })
  })

  it('adBlockToggled action', () => {
    assert.deepEqual(actions.adBlockToggled(), {
      type: types.AD_BLOCK_TOGGLED
    })
  })

  it('trackingProtectionToggled action', () => {
    assert.deepEqual(actions.trackingProtectionToggled(), {
      type: types.TRACKING_PROTECTION_TOGGLED
    })
  })

  it('resourceBlocked action', () => {
    const details = {
      blockType: 'adBlock',
      tabId: 2
    }
    assert.deepEqual(actions.resourceBlocked(details), {
      type: types.RESOURCE_BLOCKED,
      details
    })
  })
})
