import React, { Component } from 'react';
import {
  View,
  Text,
  FlatList,
  ActivityIndicator,
  NativeModules,
} from 'react-native';
import { ListItem, SearchBar, Avatar } from 'react-native-elements';
import RandomUser from './RandomUser.json';

class FlatListDemo extends Component {
  constructor(props) {
    super(props);

    this.state = {
      loading: false,
      data: [],
      error: null,
    };

    this.arrayholder = [];
  }

  componentDidMount() {
    NativeModules.TTIModule.componentDidMount(Date.now());
    this.makeRemoteRequest();
  }

  makeRemoteRequest = () => {
    this.setState({ loading: true });

    // fetch(`https://randomuser.me/api/?&results=20`)
    new Promise((resolve, reject) => {
      resolve(RandomUser);
    })
      .then((res) => {
        this.setState({
          data: res.results,
          error: res.error || null,
          loading: false,
        });
        this.arrayholder = res.results;
      })
      .catch((error) => {
        this.setState({ error, loading: false });
      });
  };

  renderSeparator = () => {
    return (
      <View
        style={{
          height: 1,
          width: '100%',
          backgroundColor: '#CED0CE',
        }}
      />
    );
  };

  searchFilterFunction = (text) => {
    this.setState({
      value: text,
    });

    const newData = this.arrayholder.filter((item) => {
      const itemData = `${item.name.title.toUpperCase()} ${item.name.first.toUpperCase()} ${item.name.last.toUpperCase()}`;
      const textData = text.toUpperCase();

      return itemData.indexOf(textData) > -1;
    });
    this.setState({
      data: newData,
    });
  };

  renderHeader = () => {
    return (
      <SearchBar
        placeholder="Type Here..."
        lightTheme
        round
        onChangeText={(text) => this.searchFilterFunction(text)}
        autoCorrect={false}
        value={this.state.value}
      />
    );
  };

  render() {
    if (this.state.loading) {
      return (
        <View
          style={{ flex: 1, alignItems: 'center', justifyContent: 'center' }}
        >
          <ActivityIndicator />
        </View>
      );
    }
    return (
      <View style={{ flex: 1 }}>
        <FlatList
          style={{ flexGrow: 0 }}
          data={this.state.data}
          renderItem={({ item }) => {
            return (
              <View>
                <Text style={{ fontSize: 20 }}>
                  {item.name.first} {item.name.last}
                </Text>
                <Text style={{ fontSize: 10 }}>{item.email}</Text>
              </View>
            );
          }}
          keyExtractor={(item) => item.email}
          ItemSeparatorComponent={this.renderSeparator}
        />
      </View>
    );
  }
}

export default FlatListDemo;
