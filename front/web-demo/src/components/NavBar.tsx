import { NavLink } from "react-router-dom";

interface Props {
  bg_color: string;
  text_color: string;
  text: string;
  navbar_items: Array<{ id: number; link: string; icon: string; name: string }>;
}

const NavBar = ({ navbar_items = [], bg_color, text_color, text }: Props) => {
  return (
    <div>
      <div className="navbar" style={{ backgroundColor: bg_color, color: text_color }}>
        <div className="flex-none">
          <div className="drawer">
            <input id="my-drawer" type="checkbox" className="drawer-toggle" />
            <div className="drawer-content">
              {/* Page content here */}
              <label
                htmlFor="my-drawer"
                className="btn btn-ghost drawer-button"
              >
                <svg
                  xmlns="http://www.w3.org/2000/svg"
                  fill="none"
                  viewBox="0 0 24 24"
                  className="inline-block h-5 w-5 stroke-current"
                >
                  {" "}
                  <path
                    strokeLinecap="round"
                    strokeLinejoin="round"
                    strokeWidth="2"
                    d="M4 6h16M4 12h16M4 18h16"
                  ></path>{" "}
                </svg>
              </label>
            </div>
            <div className="drawer-side">
              <label
                htmlFor="my-drawer"
                aria-label="close sidebar"
                className="drawer-overlay"
              ></label>
              <ul className="menu bg-base-200 text-base-content min-h-full w-80 p-4 absolute top-16">
                {/* Sidebar content here */}
                {navbar_items.map((item) => (
                  <li key={item.id}>
                    <NavLink to={item.link}>
                      <i className={item.icon}></i> &emsp; {item.name}
                    </NavLink>
                  </li>
                ))}
              </ul>
            </div>
          </div>
        </div>
        <div className="mx-2 flex-1 px-2 text-xl font-semibold"> {text} </div>
      </div>
    </div >
  );
};

export default NavBar;
