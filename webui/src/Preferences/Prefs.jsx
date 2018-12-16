import React, { Component } from 'react';
import { prefsStore } from './PrefsStore';
import { observer } from 'mobx-react';

class Prefs extends Component {
  render() {
    this.store = prefsStore;
    return (
      <div class="Prefs">
        <h2>Einstellungen</h2>
        WLAN, Max Lautstärke, LED Helligkeit
      </div>
    );
  }
}

export default observer(Prefs);
