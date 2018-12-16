import React, { Component } from 'react';
import './App.css';
import Prefs from './Preferences/Prefs';

class App extends Component {
  render() {
    return (
      <div className="App">
        <h1>H&ouml;rb&auml;r</h1>
        <Prefs />
      </div>
    );
  }
}

export default App;
