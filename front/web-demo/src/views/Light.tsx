import { useState, ChangeEvent } from "react";
import axios from "axios";

const Light = () => {
  const [RedsliderValue, setRedSliderValue] = useState(128);
  const [GreensliderValue, setGreenSliderValue] = useState(128);
  const [BluesliderValue, setBlueSliderValue] = useState(128);
  const [Color, setColor] = useState({ red: 128, green: 128, blue: 128 });
  const rgbString = `rgb(${Color.red}, ${Color.green}, ${Color.blue})`;

  const checkValue = (event: ChangeEvent<HTMLInputElement>) => {
    let value = Number(event.target.value);
    if (255 < value) value = 255;
    return value;
  };

  const handleRedSlider = (event: ChangeEvent<HTMLInputElement>) => {
    const value = checkValue(event);
    setRedSliderValue(value);
    setColor((C) => ({ ...C, red: value })); // using an updater function
  };

  const handleGreenSlider = (event: ChangeEvent<HTMLInputElement>) => {
    const value = checkValue(event);
    setGreenSliderValue(value);
    setColor((C) => ({ ...C, green: value }));
  };

  const handleBlueSlider = (event: ChangeEvent<HTMLInputElement>) => {
    const value = checkValue(event);
    setBlueSliderValue(value);
    setColor((C) => ({ ...C, blue: value }));
  };

  const handleSetColor = () => {
    axios.post('/api/v1/light/brightness', {
      red: RedsliderValue,
      green: GreensliderValue,
      blue: BluesliderValue
    })
    .then(res => console.log(res))
    .catch(err => console.log(err))
  }

  const items = [
    { value: RedsliderValue, onchange: handleRedSlider, slider_color: "red" },
    {
      value: GreensliderValue,
      onchange: handleGreenSlider,
      slider_color: "green",
    },
    {
      value: BluesliderValue,
      onchange: handleBlueSlider,
      slider_color: "blue",
    },
  ];

  return (
    <>
      <div className="color-component">
        <div
          className="color-display"
          style={{ backgroundColor: rgbString }}
        ></div>
      </div>
      {items.map((item, index) => (
        <div
          className="flex flex-cols justify-center items-center mt-15"
          key={index}
        >
          <input
            type="range"
            min={0}
            max="255"
            value={item.value}
            onChange={item.onchange}
            className="range range-xs"
            style={
              {
                "--range-bg": item.slider_color,
                "--range-fill": "0",
              } as React.CSSProperties
            }
          />
          <input
            type="number"
            className="input input-sm mx-4 px-4 w-32"
            onChange={item.onchange}
            value={item.value}
          />
        </div>
      ))}
      <div className="flex justify-center items-center mt-5">
        <button type="submit" className="btn" onClick={handleSetColor}>
          Submit
        </button>
      </div>
    </>
  );
};

export default Light;
