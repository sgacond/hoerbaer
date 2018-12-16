import React, { Component } from 'react';
import { observer } from 'mobx-react';

class Slots extends Component {
  render() {
    return (
      <div class="Prefs">
        <h2>Einstellungen</h2>
        WLAN, Max Lautstärke, LED Helligkeit
      </div>
    );
  }
}

export default observer(Slots);
