import {SidebarNav} from 'components/sidebar-nav/sidebar-nav';
import {shallow} from 'enzyme';
import * as React from 'react';
import {BrowserRouter as Router, Route} from 'react-router-dom';

import {SubdomainApp} from './subdomain-app';

describe('<SubdomainApp/> test', () => {
  it.skip('should have correct routes', () => {
    const app = shallow(<SubdomainApp />);

    expect(app.find(Route)).toHaveLength(4);
  });
});
