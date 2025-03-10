import "./App.css";
import Home from "./views/Home";
import Chart from "./views/Chart";
import Light from "./views/Light";
import NavBar from "./components/NavBar";
import Footer from "./components/Footer";
import { Routes, Route } from "react-router-dom";

function App() {
  const nav_items = [
    { id: 1, link: "/", icon: "fas fa-home", name: "Home" },
    { id: 2, link: "/chart", icon: "fas fa-line-chart", name: "Chart" },
    { id: 3, link: "/light", icon: "fas fa-lightbulb", name: "Light" },
  ];

  let footer_text = "ACME Industries Ltd";

  return (
    <div>
      <NavBar navbar_items={nav_items} bg_color="red" text_color="white" text="ESP Home" />
      <Routes>
        <Route path="/" element={<Home />} />
        <Route path="/chart" element={<Chart />} />
        <Route path="/light" element={<Light />} />
      </Routes>
      <Footer
        FooterColor="grey"
        FooterText={footer_text}
        TextColor="white"
      />
    </div>
  );
}

export default App;
